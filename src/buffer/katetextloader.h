/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATE_TEXTLOADER_H
#define KATE_TEXTLOADER_H

#include <QCryptographicHash>
#include <QFile>
#include <QMimeDatabase>
#include <QString>

// on the fly compression
#include <KCompressionDevice>

namespace Kate
{
/**
 * loader block size, load 256 kb at once per default
 * if file size is smaller, fall back to file size
 * must be a multiple of 2
 */
static const qint64 KATE_FILE_LOADER_BS = 256 * 1024;

/**
 * File Loader, will handle reading of files + detecting encoding
 */
class TextLoader
{
public:
    /**
     * Construct file loader for given file.
     * @param filename file to open
     * @param proberType prober type
     */
    TextLoader(const QString &filename, KEncodingProber::ProberType proberType)
        : m_codec(nullptr)
        , m_eof(false) // default to not eof
        , m_lastWasEndOfLine(true) // at start of file, we had a virtual newline
        , m_lastWasR(false) // we have not found a \r as last char
        , m_position(0)
        , m_lastLineStart(0)
        , m_eol(TextBuffer::eolUnknown) // no eol type detected atm
        , m_buffer(KATE_FILE_LOADER_BS, 0)
        , m_digest(QCryptographicHash::Sha1)
        , m_converterState(nullptr)
        , m_bomFound(false)
        , m_firstRead(true)
        , m_proberType(proberType)
        , m_fileSize(0)
    {
        // try to get mimetype for on the fly decompression, don't rely on filename!
        QFile testMime(filename);
        m_mimeType = QMimeDatabase().mimeTypeForFileNameAndData(filename, &testMime).name();
        m_fileSize = testMime.size();

        // construct filter device
        KCompressionDevice::CompressionType compressionType = KCompressionDevice::compressionTypeForMimeType(m_mimeType);
        m_file = new KCompressionDevice(filename, compressionType);
    }

    /**
     * Destructor
     */
    ~TextLoader()
    {
        delete m_file;
        delete m_converterState;
    }

    /**
     * open file with given codec
     * @param codec codec to use, if 0, will do some auto-detect or fallback
     * @return success
     */
    bool open(QTextCodec *codec)
    {
        m_codec = codec;
        m_eof = false;
        m_lastWasEndOfLine = true;
        m_lastWasR = false;
        m_position = 0;
        m_lastLineStart = 0;
        m_eol = TextBuffer::eolUnknown;
        m_text.clear();
        delete m_converterState;
        m_converterState = new QTextCodec::ConverterState(QTextCodec::DefaultConversion);
        m_bomFound = false;
        m_firstRead = true;

        // init the hash with the git header
        const QString header = QStringLiteral("blob %1").arg(m_fileSize);
        m_digest.reset();
        m_digest.addData(header.toLatin1() + '\0');

        // if already opened, close the file...
        if (m_file->isOpen()) {
            m_file->close();
        }

        return m_file->open(QIODevice::ReadOnly);
    }

    /**
     * end of file reached?
     * @return end of file reached
     */
    bool eof() const
    {
        return m_eof && !m_lastWasEndOfLine && (m_lastLineStart == m_text.length());
    }

    /**
     * Detected end of line mode for this file.
     * Detected during reading, is valid after complete file is read.
     * @return eol mode of this file
     */
    TextBuffer::EndOfLineMode eol() const
    {
        return m_eol;
    }

    /**
     * BOM found?
     * @return byte order mark found?
     */
    bool byteOrderMarkFound() const
    {
        return m_bomFound;
    }

    /**
     * mime type used to create filter dev
     * @return mime-type of filter device
     */
    const QString &mimeTypeForFilterDev() const
    {
        return m_mimeType;
    }

    /**
     * internal Unicode data array
     * @return internal Unicode data
     */
    const QChar *unicode() const
    {
        return m_text.unicode();
    }

    /**
     * Get codec for this loader
     * @return currently in use codec of this loader
     */
    QTextCodec *textCodec() const
    {
        return m_codec;
    }

    /**
     * read a line, return length + offset in Unicode data
     * @param offset offset into internal Unicode data for read line
     * @param length length of read line
     * @return true if no encoding errors occurred
     */
    bool readLine(int &offset, int &length)
    {
        length = 0;
        offset = 0;
        bool encodingError = false;

        static const QLatin1Char cr(QLatin1Char('\r'));
        static const QLatin1Char lf(QLatin1Char('\n'));

        /**
         * did we read two time but got no stuff? encoding error
         * fixes problem with one character latin-1 files, which lead to crash otherwise!
         * bug 272579
         */
        bool failedToConvertOnce = false;
        /**
         * keep track if we have found BOM so that failedToConvertOnce is not erroneously set to true
         * BUG: 440359
         */
        bool bomPreviouslyFound = m_bomFound;

        /**
         * reading loop
         */
        while (m_position <= m_text.length()) {
            if (m_position == m_text.length()) {
                // try to load more text if something is around
                if (!m_eof) {
                    // kill the old lines...
                    m_text.remove(0, m_lastLineStart);

                    // try to read new data
                    const int c = m_file->read(m_buffer.data(), m_buffer.size());

                    // if any text is there, append it....
                    if (c > 0) {
                        // update hash sum
                        m_digest.addData(m_buffer.data(), c);

                        // detect byte order marks & codec for byte order marks on first read
                        int bomBytes = 0;
                        if (m_firstRead) {
                            // use first 16 bytes max to allow BOM detection of codec
                            QByteArray bom(m_buffer.data(), qMin(16, c));
                            QTextCodec *codecForByteOrderMark = QTextCodec::codecForUtfText(bom, nullptr);

                            // if codecForByteOrderMark != null, we found a BOM!
                            // BUT we only capture BOM if no codec was set, or the BOM encodes the same codec as m_codec.
                            // These additional checks are necessary so that the (coincidentally matching) BOM characters won't be eaten for non-UTF encodings
                            // TODO: support BOMs for other encodings? (see e.g. https://en.wikipedia.org/wiki/Byte_order_mark#Byte_order_marks_by_encoding)
                            if (codecForByteOrderMark && (!m_codec || codecForByteOrderMark->mibEnum() == m_codec->mibEnum())) {
                                m_bomFound = true;

                                // eat away the different boms!
                                const int mib = codecForByteOrderMark->mibEnum();
                                if (mib == 106) { // utf8
                                    bomBytes = 3;
                                } else if (mib == 1013 || mib == 1014 || mib == 1015) { // utf16
                                    bomBytes = 2;
                                } else if (mib == 1017 || mib == 1018 || mib == 1019) { // utf32
                                    bomBytes = 4;
                                }
                            }

                            /**
                             * if no codec given, do autodetection
                             */
                            if (!m_codec) {
                                /**
                                 * byte order said something about encoding?
                                 */
                                if (codecForByteOrderMark) {
                                    m_codec = codecForByteOrderMark;
                                } else {
                                    /**
                                     * no Unicode BOM found, trigger prober
                                     */

                                    /**
                                     * first: try to get HTML header encoding
                                     */
                                    if (QTextCodec *codecForHtml = QTextCodec::codecForHtml(m_buffer, nullptr)) {
                                        m_codec = codecForHtml;
                                    }

                                    /**
                                     * else: use KEncodingProber
                                     */
                                    else {
                                        KEncodingProber prober(m_proberType);
                                        prober.feed(m_buffer.constData(), c);

                                        // we found codec with some confidence?
                                        if (prober.confidence() > 0.5) {
                                            m_codec = QTextCodec::codecForName(prober.encoding());
                                        }
                                    }

                                    // no codec, no chance, encoding error
                                    if (!m_codec) {
                                        return false;
                                    }
                                }
                            }

                            m_firstRead = false;
                        }

                        // detect broken encoding, we did before use QTextCodec::ConvertInvalidToNull and check for 0 chars
                        // this lead to issues with files containing 0 chars, therefore use the invalidChars field of the state
                        Q_ASSERT(m_codec);
                        QString unicode = m_codec->toUnicode(m_buffer.constData() + bomBytes, c - bomBytes, m_converterState);
                        encodingError = encodingError || m_converterState->invalidChars;
                        m_text.append(unicode);
                    }

                    // is file completely read ?
                    m_eof = (c == -1) || (c == 0);

                    // recalc current pos and last pos
                    m_position -= m_lastLineStart;
                    m_lastLineStart = 0;
                }

                // oh oh, end of file, escape !
                if (m_eof && (m_position == m_text.length())) {
                    m_lastWasEndOfLine = false;

                    // line data
                    offset = m_lastLineStart;
                    length = m_position - m_lastLineStart;

                    m_lastLineStart = m_position;

                    return !encodingError && !failedToConvertOnce;
                }

                // empty? try again
                if (m_position == m_text.length()) {
                    if (!bomPreviouslyFound && m_bomFound) {
                        // BOM was processed above, so we didn't fail to convert
                        bomPreviouslyFound = true;
                    } else {
                        failedToConvertOnce = true;
                    }
                    continue;
                }
            }

            QChar current_char = m_text.at(m_position);
            if (current_char == lf) {
                m_lastWasEndOfLine = true;

                if (m_lastWasR) {
                    m_lastLineStart++;
                    m_lastWasR = false;
                    m_eol = TextBuffer::eolDos;
                } else {
                    // line data
                    offset = m_lastLineStart;
                    length = m_position - m_lastLineStart;

                    m_lastLineStart = m_position + 1;
                    m_position++;

                    // only win, if not dos!
                    if (m_eol != TextBuffer::eolDos) {
                        m_eol = TextBuffer::eolUnix;
                    }

                    return !encodingError;
                }
            } else if (current_char == cr) {
                m_lastWasEndOfLine = true;
                m_lastWasR = true;

                // line data
                offset = m_lastLineStart;
                length = m_position - m_lastLineStart;

                m_lastLineStart = m_position + 1;
                m_position++;

                // should only win of first time!
                if (m_eol == TextBuffer::eolUnknown) {
                    m_eol = TextBuffer::eolMac;
                }

                return !encodingError;
            } else if (current_char == QChar::LineSeparator) {
                m_lastWasEndOfLine = true;

                // line data
                offset = m_lastLineStart;
                length = m_position - m_lastLineStart;

                m_lastLineStart = m_position + 1;
                m_position++;

                return !encodingError;
            } else {
                m_lastWasEndOfLine = false;
                m_lastWasR = false;
            }

            m_position++;
        }

        return !encodingError;
    }

    QByteArray digest()
    {
        return m_digest.result();
    }

private:
    QTextCodec *m_codec;
    bool m_eof;
    bool m_lastWasEndOfLine;
    bool m_lastWasR;
    int m_position;
    int m_lastLineStart;
    TextBuffer::EndOfLineMode m_eol;
    QString m_mimeType;
    QIODevice *m_file;
    QByteArray m_buffer;
    QCryptographicHash m_digest;
    QString m_text;
    QTextCodec::ConverterState *m_converterState;
    bool m_bomFound;
    bool m_firstRead;
    KEncodingProber::ProberType m_proberType;
    quint64 m_fileSize;
};

}

#endif
