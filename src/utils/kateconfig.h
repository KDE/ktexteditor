/* This file is part of the KDE libraries
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_CONFIG_H
#define KATE_CONFIG_H

#include <ktexteditor_export.h>

#include <ktexteditor/markinterface.h>
#include "ktexteditor/view.h"
#include <KEncodingProber>

#include <functional>
#include <map>
#include <memory>

#include <QBitRef>
#include <QColor>
#include <QObject>
#include <QVector>
#include <QFontMetricsF>

class KConfigGroup;
namespace KTextEditor { class ViewPrivate; }
namespace KTextEditor { class DocumentPrivate; }
class KateRenderer;

namespace KTextEditor {
    class EditorPrivate;
}

class KConfig;

class QTextCodec;

/**
 * Base Class for the Kate Config Classes
 */
class KTEXTEDITOR_EXPORT KateConfig
{
public:
    /**
     * start some config changes
     * this method is needed to init some kind of transaction
     * for config changes, update will only be done once, at
     * configEnd() call
     */
    void configStart();

    /**
     * end a config change transaction, update the concerned
     * documents/views/renderers
     */
    void configEnd();

    /**
     * global config?
     * @return global config object?
     */
    bool isGlobal() const
    {
        return !m_parent;
    }

    /**
     * All known config keys.
     * This will use the knowledge about all registered keys of the global object.
     * @return all known config keys
     */
    QStringList configKeys() const
    {
        return m_parent ? m_parent->configKeys() : *m_configKeys.get();
    }

    /**
     * Get a config value.
     * @param key config key, aka enum from KateConfig* classes
     * @return value for the wanted key, will assert if key is not valid
     */
    QVariant value(const int key) const;

    /**
     * Set a config value.
     * Will assert if key is invalid.
     * Might not alter the value if given value fails validation.
     * @param key config key, aka enum from KateConfig* classes
     * @param value value to set
     * @return value set?
     */
    bool setValue(const int key, const QVariant &value);

    /**
     * Get a config value for the string key.
     * @param key config key, aka commandName from KateConfig* classes
     * @return value for the wanted key, will return invalid variant if key invalid
     */
    QVariant value(const QString &key) const;

    /**
     * Set a config value.
     * Will ignore set if key not valid
     * Might not alter the value if given value fails validation.
     * @param key config key, aka commandName from KateConfig* classes
     * @param value value to set
     * @return value set?
     */
    bool setValue(const QString &key, const QVariant &value);

protected:
    /**
     * Construct a KateConfig.
     * @param parent parent config object, if any
     */
    KateConfig(const KateConfig *parent = nullptr);

    /**
     * Virtual Destructor
     */
    virtual ~KateConfig();

    /**
     * One config entry.
     */
    class ConfigEntry {
    public:
        /**
         * Construct one config entry.
         * @param enumId value of the enum for this config entry
         * @param configId value of the key for the KConfig file for this config entry
         * @param command command name
         * @param defaultVal default value
         * @param valid validator function, default none
         */
        ConfigEntry(int enumId, const char *configId, QString command, QVariant defaultVal, std::function<bool(const QVariant &)> valid = nullptr)
            : enumKey(enumId)
            , configKey(configId)
            , commandName(command)
            , defaultValue(defaultVal)
            , value(defaultVal)
            , validator(valid)
        {
        }

        /**
         * Enum key for this config entry, shall be unique
         */
        const int enumKey;

        /**
         * KConfig entry key for this config entry, shall be unique in its group
         * e.g. "Tab Width"
         */
         const char * const configKey;

         /**
          * Command name as used in e.g. ConfigInterface or modeline/command line
          * e.g. tab-width
          */
         const QString commandName;

        /**
         * Default value if nothing configured
         */
        const QVariant defaultValue;

        /**
         * concrete value, per default == defaultValue
         */
        QVariant value;

        /**
         * a validator function, only if this returns true we accept a value for the entry
         * is ignored if not set
         */
        std::function<bool(const QVariant &)> validator;
    };

    /**
     * Read all config entries from given config group
     * @param config config group to read from
     */
    void readConfigEntries(const KConfigGroup &config);

    /**
     * Write all config entries to given config group
     * @param config config group to write to
     */
    void writeConfigEntries(KConfigGroup &config) const;

    /**
     * Register a new config entry.
     * Used by the sub classes to register all there known ones.
     * @param entry new entry to add
     */
    void addConfigEntry(ConfigEntry &&entry);

    /**
     * Finalize the config entries.
     * Called by the sub classes after all entries are registered
     */
    void finalizeConfigEntries();

    /**
     * do the real update
     */
    virtual void updateConfig() = 0;

private:
    /**
     * Get full map of config entries, aka the m_configEntries of the top config object
     * @return full map with all config entries
     */
    const std::map<int, ConfigEntry> &fullConfigEntries () const
    {
        return m_parent ? m_parent->fullConfigEntries() : m_configEntries;
    }
    /**
     * Get hash of config entries, aka the m_configKeyToEntry of the top config object
     * @return full hash with all config entries
     */
    const QHash<QString, const ConfigEntry *> &fullConfigKeyToEntry () const
    {
        return m_parent ? m_parent->fullConfigKeyToEntry() : *m_configKeyToEntry.get();
    }

private:
    /**
     * parent config object, if any
     */
    const KateConfig * const m_parent = nullptr;

    /**
     * recursion depth
     */
    uint configSessionNumber = 0;

    /**
     * is a config session running
     */
    bool configIsRunning = false;

    /**
     * two cases:
     *   - we have m_parent == nullptr => this contains all known config entries
     *   - we have m_parent != nullptr => this contains all set config entries for this level of configuration
     *
     * uses a map ATM for deterministic iteration e.g. for read/writeConfig
     */
    std::map<int, ConfigEntry> m_configEntries;

    /**
     * All known config keys, filled only for the object with m_parent == nullptr
     */
    std::unique_ptr<QStringList> m_configKeys;

    /**
     * Hash of config keys => config entry, filled only for the object with m_parent == nullptr
     */
    std::unique_ptr<QHash<QString, const ConfigEntry *>> m_configKeyToEntry;
};

class KTEXTEDITOR_EXPORT KateGlobalConfig : public KateConfig
{
private:
    friend class KTextEditor::EditorPrivate;

    /**
     * only used in KTextEditor::EditorPrivate for the static global fallback !!!
     */
    KateGlobalConfig();

public:
    static KateGlobalConfig *global()
    {
        return s_global;
    }

    /**
     * Known config entries
     */
    enum ConfigEntryTypes {
        /**
         * Encoding prober
         */
        EncodingProberType,

        /**
         * Fallback encoding
         */
        FallbackEncoding
    };

public:
    /**
     * Read config from object
     */
    void readConfig(const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig(KConfigGroup &config);

protected:
    void updateConfig() override;

public:
    KEncodingProber::ProberType proberType() const
    {
        return KEncodingProber::ProberType(value(EncodingProberType).toInt());
    }

    bool setProberType(KEncodingProber::ProberType type)
    {
        return setValue(EncodingProberType, type);
    }

    /**
     * Fallback codec.
     * Based on fallback encoding.
     * @return fallback codec
     */
    QTextCodec *fallbackCodec() const;

    QString fallbackEncoding() const
    {
        return value(FallbackEncoding).toString();
    }

    bool setFallbackEncoding(const QString &encoding)
    {
        return setValue(FallbackEncoding, encoding);
    }

private:
    static KateGlobalConfig *s_global;
};

class KTEXTEDITOR_EXPORT KateDocumentConfig : public KateConfig
{
private:
    friend class KTextEditor::EditorPrivate;

    KateDocumentConfig();

public:
    /**
     * Construct a DocumentConfig
     */
    explicit KateDocumentConfig(KTextEditor::DocumentPrivate *doc);

    inline static KateDocumentConfig *global()
    {
        return s_global;
    }

    /**
     * Known config entries
     */
    enum ConfigEntryTypes {
        /**
         * Tabulator width
         */
        TabWidth,

        /**
         * Indentation width
         */
        IndentationWidth,

        /**
         * On-the-fly spellcheck enabled?
         */
        OnTheFlySpellCheck,

        /**
         * Indent pasted text?
         */
        IndentOnTextPaste,

        /**
         * Replace tabs with spaces?
         */
        ReplaceTabsWithSpaces,

        /**
         * Backup files for local files?
         */
        BackupOnSaveLocal,

        /**
         * Backup files for remote files?
         */
        BackupOnSaveRemote,

        /**
         * Prefix for backup files
         */
        BackupOnSavePrefix,

        /**
         * Suffix for backup files
         */
        BackupOnSaveSuffix,

        /**
         * Indentation mode, like "normal"
         */
        IndentationMode,

        /**
         * Tab handling, like indent, insert tab, smart
         */
        TabHandlingMode,

        /**
         * Static word wrap?
         */
        StaticWordWrap,

        /**
         * Static word wrap column
         */
        StaticWordWrapColumn,

        /**
         * PageUp/Down moves cursor?
         */
        PageUpDownMovesCursor,

        /**
         * Smart Home key?
         */
        SmartHome,

        /**
         * Show Tabs?
         */
        ShowTabs,

        /**
         * Indent on tab?
         */
        IndentOnTab,

        /**
         * Keep extra space?
         */
        KeepExtraSpaces,

        /**
         * Backspace key indents?
         */
        BackspaceIndents,

        /**
         * Show spaces mode like none, all, ...
         */
        ShowSpacesMode,

        /**
         * Trailing Marker Size
         */
        TrailingMarkerSize,

        /**
         * Remove spaces mode
         */
        RemoveSpacesMode,

        /**
         * Ensure newline at end of file
         */
        NewlineAtEOF,

        /**
         * Overwrite mode?
         */
        OverwriteMode,

        /**
         * Encoding
         */
        Encoding,

        /**
         * End of line mode: dos, mac, unix
         */
        EndOfLine,

        /**
         * Allow EOL detection
         */
        AllowEndOfLineDetection,

        /**
         * Use Byte Order Mark
         */
        ByteOrderMark,

        /**
         * Swap file mode
         */
        SwapFile,

        /**
         * Swap file directory
         */
        SwapFileDirectory,

        /**
         * Swap file sync interval
         */
        SwapFileSyncInterval,

        /**
         * Line length limit
         */
        LineLengthLimit
    };

public:
    /**
     * Read config from object
     */
    void readConfig(const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig(KConfigGroup &config);

protected:
    void updateConfig() override;

public:
    int tabWidth() const
    {
        return value(TabWidth).toInt();
    }

    void setTabWidth(int tabWidth)
    {
        setValue(TabWidth, QVariant(tabWidth));
    }

    int indentationWidth() const
    {
        return value(IndentationWidth).toInt();
    }

    void setIndentationWidth(int indentationWidth)
    {
        setValue(IndentationWidth, QVariant(indentationWidth));
    }

    bool onTheFlySpellCheck() const
    {
        return value(OnTheFlySpellCheck).toBool();
    }

    void setOnTheFlySpellCheck(bool on)
    {
        setValue(OnTheFlySpellCheck, QVariant(on));
    }

    bool indentPastedText() const
    {
        return value(IndentOnTextPaste).toBool();
    }

    void setIndentPastedText(bool on)
    {
        setValue(IndentOnTextPaste, QVariant(on));
    }

    bool replaceTabsDyn() const
    {
        return value(ReplaceTabsWithSpaces).toBool();
    }

    void setReplaceTabsDyn(bool on)
    {
        setValue(ReplaceTabsWithSpaces, QVariant(on));
    }

    bool backupOnSaveLocal() const
    {
        return value(BackupOnSaveLocal).toBool();
    }

    void setBackupOnSaveLocal(bool on)
    {
        setValue(BackupOnSaveLocal, QVariant(on));
    }

    bool backupOnSaveRemote() const
    {
        return value(BackupOnSaveRemote).toBool();
    }

    void setBackupOnSaveRemote(bool on)
    {
        setValue(BackupOnSaveRemote, QVariant(on));
    }

    QString backupPrefix() const
    {
        return value(BackupOnSavePrefix).toString();
    }

    void setBackupPrefix(const QString &prefix)
    {
        setValue(BackupOnSavePrefix, QVariant(prefix));
    }

    QString backupSuffix() const
    {
        return value(BackupOnSaveSuffix).toString();
    }

    void setBackupSuffix(const QString &suffix)
    {
        setValue(BackupOnSaveSuffix, QVariant(suffix));
    }

    QString indentationMode() const
    {
        return value(IndentationMode).toString();
    }

    void setIndentationMode(const QString &identationMode)
    {
        setValue(IndentationMode, identationMode);
    }

    enum TabHandling {
        tabInsertsTab = 0,
        tabIndents = 1,
        tabSmart = 2      //!< indents in leading space, otherwise inserts tab
    };

    enum WhitespaceRendering {
        None,
        Trailing,
        All
    };

    int tabHandling() const
    {
        return value(TabHandlingMode).toInt();
    }

    void setTabHandling(int tabHandling)
    {
        setValue(TabHandlingMode, tabHandling);
    }

    bool wordWrap() const
    {
        return value(StaticWordWrap).toBool();
    }

    void setWordWrap(bool on)
    {
        setValue(StaticWordWrap, on);
    }

    int wordWrapAt() const
    {
        return value(StaticWordWrapColumn).toInt();
    }

    void setWordWrapAt(int col)
    {
        setValue(StaticWordWrapColumn, col);
    }

    bool pageUpDownMovesCursor() const
    {
        return value(PageUpDownMovesCursor).toBool();
    }

    void setPageUpDownMovesCursor(bool on)
    {
        setValue(PageUpDownMovesCursor, on);
    }

    void setKeepExtraSpaces(bool on)
    {
        setValue(KeepExtraSpaces, on);
    }

    bool keepExtraSpaces() const
    {
        return value(KeepExtraSpaces).toBool();
    }

    void setBackspaceIndents(bool on)
    {
        setValue(BackspaceIndents, on);
    }

    bool backspaceIndents() const
    {
        return value(BackspaceIndents).toBool();
    }

    void setSmartHome(bool on)
    {
        setValue(SmartHome, on);
    }

    bool smartHome() const
    {
        return value(SmartHome).toBool();
    }

    void setShowTabs(bool on)
    {
        setValue(ShowTabs, on);
    }

    bool showTabs() const
    {
        return value(ShowTabs).toBool();
    }

    void setShowSpaces(WhitespaceRendering mode)
    {
        setValue(ShowSpacesMode, mode);
    }

    WhitespaceRendering showSpaces() const
    {
        return WhitespaceRendering(value(ShowSpacesMode).toInt());
    }

    void setMarkerSize(int markerSize)
    {
        setValue(TrailingMarkerSize, markerSize);
    }

    int markerSize() const
    {
        return value(TrailingMarkerSize).toInt();
    }

    /**
     * Remove trailing spaces on save.
     * triState = 0: never remove trailing spaces
     * triState = 1: remove trailing spaces of modified lines (line modification system)
     * triState = 2: remove trailing spaces in entire document
     */
    void setRemoveSpaces(int triState)
    {
        setValue(RemoveSpacesMode, triState);
    }

    int removeSpaces() const
    {
        return value(RemoveSpacesMode).toInt();
    }

    void setNewLineAtEof(bool on)
    {
        setValue(NewlineAtEOF, on);
    }

    bool newLineAtEof() const
    {
        return value(NewlineAtEOF).toBool();
    }

    void setOvr(bool on)
    {
        setValue(OverwriteMode, on);
    }

    bool ovr() const
    {
        return value(OverwriteMode).toBool();
    }

    void setTabIndents(bool on)
    {
        setValue(IndentOnTab, on);
    }

    bool tabIndentsEnabled() const
    {
        return value(IndentOnTab).toBool();
    }

    /**
     * Get current text codec.
     * Based on current set encoding
     * @return current text codec.
     */
    QTextCodec *codec() const;

    QString encoding() const
    {
        return value(Encoding).toString();
    }

    bool setEncoding(const QString &encoding)
    {
        return setValue(Encoding, encoding);
    }

    enum Eol {
        eolUnix = 0,
        eolDos = 1,
        eolMac = 2
    };

    int eol() const
    {
        return value(EndOfLine).toInt();
    }

    /**
     * Get current end of line string.
     * Based on current set eol mode.
     * @return current end of line string
     */
    QString eolString();

    void setEol(int mode)
    {
        setValue(EndOfLine, mode);
    }

    bool bom() const
    {
        return value(ByteOrderMark).toBool();
    }

    void setBom(bool bom)
    {
        setValue(ByteOrderMark, bom);
    }

    bool allowEolDetection() const
    {
        return value(AllowEndOfLineDetection).toBool();
    }

    void setAllowEolDetection(bool on)
    {
        setValue(AllowEndOfLineDetection, on);
    }

    QString swapDirectory() const
    {
        return value(SwapFileDirectory).toString();
    }

    void setSwapDirectory(const QString &directory)
    {
        setValue(SwapFileDirectory, directory);
    }

    enum SwapFileMode {
        DisableSwapFile = 0,
        EnableSwapFile,
        SwapFilePresetDirectory
    };

    SwapFileMode swapFileMode() const
    {
        return SwapFileMode(value(SwapFile).toInt());
    }

    void setSwapFileMode(int mode)
    {
        setValue(SwapFile, mode);
    }

    int swapSyncInterval() const
    {
        return value(SwapFileSyncInterval).toInt();
    }

    void setSwapSyncInterval(int interval)
    {
        setValue(SwapFileSyncInterval, interval);
    }

    int lineLengthLimit() const
    {
        return value(LineLengthLimit).toInt();
    }

    void setLineLengthLimit(int limit)
    {
        setValue(LineLengthLimit, limit);
    }

private:
    static KateDocumentConfig *s_global;
    KTextEditor::DocumentPrivate *m_doc = nullptr;
};

class KTEXTEDITOR_EXPORT KateViewConfig : public KateConfig
{
private:
    friend class KTextEditor::EditorPrivate;

    /**
     * only used in KTextEditor::EditorPrivate for the static global fallback !!!
     */
    KateViewConfig();

public:
    /**
     * Construct a DocumentConfig
     */
    explicit KateViewConfig(KTextEditor::ViewPrivate *view);

    /**
     * Cu DocumentConfig
     */
    ~KateViewConfig() override;

    inline static KateViewConfig *global()
    {
        return s_global;
    }

public:
    /**
     * Read config from object
     */
    void readConfig(const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig(KConfigGroup &config);

protected:
    void updateConfig() override;

public:
    bool dynWordWrapSet() const
    {
        return m_dynWordWrapSet;
    }
    bool dynWordWrap() const;
    void setDynWordWrap(bool wrap);

    bool dynWrapAtStaticMarker() const;
    void setDynWrapAtStaticMarker(bool on);

    int dynWordWrapIndicators() const;
    void setDynWordWrapIndicators(int mode);

    int dynWordWrapAlignIndent() const;
    void setDynWordWrapAlignIndent(int indent);

    bool lineNumbers() const;
    void setLineNumbers(bool on);

    bool scrollBarMarks() const;
    void setScrollBarMarks(bool on);

    bool scrollBarPreview() const;
    void setScrollBarPreview(bool on);

    bool scrollBarMiniMap() const;
    void setScrollBarMiniMap(bool on);

    bool scrollBarMiniMapAll() const;
    void setScrollBarMiniMapAll(bool on);

    int scrollBarMiniMapWidth() const;
    void setScrollBarMiniMapWidth(int width);

    /* Whether to show scrollbars */
    enum ScrollbarMode {
        AlwaysOn = 0,
        ShowWhenNeeded,
        AlwaysOff
    };

    int showScrollbars() const;
    void setShowScrollbars(int mode);

    bool iconBar() const;
    void setIconBar(bool on);

    bool foldingBar() const;
    void setFoldingBar(bool on);

    bool foldingPreview() const;
    void setFoldingPreview(bool on);

    bool lineModification() const;
    void setLineModification(bool on);

    int bookmarkSort() const;
    void setBookmarkSort(int mode);

    int autoCenterLines() const;
    void setAutoCenterLines(int lines);

    enum SearchFlags {
        IncMatchCase = 1 << 0,
        IncHighlightAll = 1 << 1,
        IncFromCursor = 1 << 2,
        PowerMatchCase = 1 << 3,
        PowerHighlightAll = 1 << 4,
        PowerFromCursor = 1 << 5,
        // PowerSelectionOnly = 1 << 6, Better not save to file // Sebastian
        PowerModePlainText = 1 << 7,
        PowerModeWholeWords = 1 << 8,
        PowerModeEscapeSequences = 1 << 9,
        PowerModeRegularExpression = 1 << 10,
        PowerUsePlaceholders = 1 << 11
    };

    long searchFlags() const;
    void setSearchFlags(long flags);

    int maxHistorySize() const;

    uint defaultMarkType() const;
    void setDefaultMarkType(uint type);

    bool allowMarkMenu() const;
    void setAllowMarkMenu(bool allow);

    bool persistentSelection() const;
    void setPersistentSelection(bool on);

    KTextEditor::View::InputMode inputMode() const;
    void setInputMode(KTextEditor::View::InputMode mode);
    void setInputModeRaw(int rawmode);

    bool viInputModeStealKeys() const;
    void setViInputModeStealKeys(bool on);

    bool viRelativeLineNumbers() const;
    void setViRelativeLineNumbers(bool on);

    // Do we still need the enum and related functions below?
    enum TextToSearch {
        Nowhere = 0,
        SelectionOnly = 1,
        SelectionWord = 2,
        WordOnly = 3,
        WordSelection = 4
    };

    bool automaticCompletionInvocation() const;
    void setAutomaticCompletionInvocation(bool on);

    bool wordCompletion() const;
    void setWordCompletion(bool on);

    bool keywordCompletion () const;
    void setKeywordCompletion (bool on);

    int wordCompletionMinimalWordLength() const;
    void setWordCompletionMinimalWordLength(int length);

    bool wordCompletionRemoveTail() const;
    void setWordCompletionRemoveTail(bool on);

    bool smartCopyCut() const;
    void setSmartCopyCut(bool on);

    bool mousePasteAtCursorPosition() const;
    void setMousePasteAtCursorPosition(bool on);

    bool scrollPastEnd() const;
    void setScrollPastEnd(bool on);

    bool foldFirstLine() const;
    void setFoldFirstLine(bool on);

    bool showWordCount() const;
    void setShowWordCount(bool on);

    bool showLineCount() const;
    void setShowLineCount(bool on);

    bool autoBrackets() const;
    void setAutoBrackets(bool on);

    bool backspaceRemoveComposed() const;
    void setBackspaceRemoveComposed(bool on);

private:
    bool m_dynWordWrap;
    bool m_dynWrapAtStaticMarker;
    int m_dynWordWrapIndicators;
    int m_dynWordWrapAlignIndent;
    bool m_lineNumbers;
    bool m_scrollBarMarks;
    bool m_scrollBarPreview;
    bool m_scrollBarMiniMap;
    bool m_scrollBarMiniMapAll;
    int  m_scrollBarMiniMapWidth;
    int  m_showScrollbars;
    bool m_iconBar;
    bool m_foldingBar;
    bool m_foldingPreview;
    bool m_lineModification;
    int m_bookmarkSort;
    int m_autoCenterLines;
    long m_searchFlags;
    int m_maxHistorySize;
    uint m_defaultMarkType;
    bool m_persistentSelection;
    KTextEditor::View::InputMode m_inputMode;
    bool m_viInputModeStealKeys;
    bool m_viRelativeLineNumbers;
    bool m_automaticCompletionInvocation;
    bool m_wordCompletion;
    bool m_keywordCompletion;
    int m_wordCompletionMinimalWordLength;
    bool m_wordCompletionRemoveTail;
    bool m_smartCopyCut;
    bool m_mousePasteAtCursorPosition = false;
    bool m_scrollPastEnd;
    bool m_foldFirstLine;
    bool m_showWordCount = false;
    bool m_showLineCount = false;
    bool m_autoBrackets;
    bool m_backspaceRemoveComposed;

    bool m_dynWordWrapSet : 1;
    bool m_dynWrapAtStaticMarkerSet : 1;
    bool m_dynWordWrapIndicatorsSet : 1;
    bool m_dynWordWrapAlignIndentSet : 1;
    bool m_lineNumbersSet : 1;
    bool m_scrollBarMarksSet : 1;
    bool m_scrollBarPreviewSet : 1;
    bool m_scrollBarMiniMapSet : 1;
    bool m_scrollBarMiniMapAllSet : 1;
    bool m_scrollBarMiniMapWidthSet : 1;
    bool m_showScrollbarsSet : 1;
    bool m_iconBarSet : 1;
    bool m_foldingBarSet : 1;
    bool m_foldingPreviewSet : 1;
    bool m_lineModificationSet : 1;
    bool m_bookmarkSortSet : 1;
    bool m_autoCenterLinesSet : 1;
    bool m_searchFlagsSet : 1;
    bool m_defaultMarkTypeSet : 1;
    bool m_persistentSelectionSet : 1;
    bool m_inputModeSet : 1;
    bool m_viInputModeStealKeysSet : 1;
    bool m_viRelativeLineNumbersSet : 1;
    bool m_automaticCompletionInvocationSet : 1;
    bool m_wordCompletionSet : 1;
    bool m_keywordCompletionSet : 1;
    bool m_wordCompletionMinimalWordLengthSet : 1;
    bool m_smartCopyCutSet : 1;
    bool m_mousePasteAtCursorPositionSet : 1;
    bool m_scrollPastEndSet : 1;
    bool m_allowMarkMenu : 1;
    bool m_wordCompletionRemoveTailSet : 1;
    bool m_foldFirstLineSet : 1;
    bool m_showWordCountSet : 1;
    bool m_showLineCountSet : 1;
    bool m_autoBracketsSet : 1;
    bool m_backspaceRemoveComposedSet : 1;

private:
    static KateViewConfig *s_global;
    KTextEditor::ViewPrivate *m_view = nullptr;
};

class KTEXTEDITOR_EXPORT KateRendererConfig : public KateConfig
{
private:
    friend class KTextEditor::EditorPrivate;

    /**
     * only used in KTextEditor::EditorPrivate for the static global fallback !!!
     */
    KateRendererConfig();

public:
    /**
     * Construct a DocumentConfig
     */
    explicit KateRendererConfig(KateRenderer *renderer);

    /**
     * Cu DocumentConfig
     */
    ~KateRendererConfig() override;

    inline static KateRendererConfig *global()
    {
        return s_global;
    }

public:
    /**
     * Read config from object
     */
    void readConfig(const KConfigGroup &config);

    /**
     * Write config to object
     */
    void writeConfig(KConfigGroup &config);

protected:
    void updateConfig() override;

public:
    const QString &schema() const;
    void setSchema(const QString &schema);

    /**
     * Reload the schema from the schema manager.
     * For the global instance, have all other instances reload.
     * Used by the schema config page to apply changes.
     */
    void reloadSchema();

    const QFont &font() const;
    const QFontMetricsF &fontMetrics() const;
    void setFont(const QFont &font);

    bool wordWrapMarker() const;
    void setWordWrapMarker(bool on);

    const QColor &backgroundColor() const;
    void setBackgroundColor(const QColor &col);

    const QColor &selectionColor() const;
    void setSelectionColor(const QColor &col);

    const QColor &highlightedLineColor() const;
    void setHighlightedLineColor(const QColor &col);

    const QColor &lineMarkerColor(KTextEditor::MarkInterface::MarkTypes type = KTextEditor::MarkInterface::markType01) const; // markType01 == Bookmark
    void setLineMarkerColor(const QColor &col, KTextEditor::MarkInterface::MarkTypes type = KTextEditor::MarkInterface::markType01);

    const QColor &highlightedBracketColor() const;
    void setHighlightedBracketColor(const QColor &col);

    const QColor &wordWrapMarkerColor() const;
    void setWordWrapMarkerColor(const QColor &col);

    const QColor &tabMarkerColor() const;
    void setTabMarkerColor(const QColor &col);

    const QColor &indentationLineColor() const;
    void setIndentationLineColor(const QColor &col);

    const QColor &iconBarColor() const;
    void setIconBarColor(const QColor &col);

    const QColor &foldingColor() const;
    void setFoldingColor(const QColor &col);

    // the line number color is used for the line numbers on the left bar
    const QColor &lineNumberColor() const;
    void setLineNumberColor(const QColor &col);
    const QColor &currentLineNumberColor() const;
    void setCurrentLineNumberColor(const QColor &col);

    // the color of the separator between line numbers and icon bar
    const QColor &separatorColor() const;
    void setSeparatorColor(const QColor &col);

    const QColor &spellingMistakeLineColor() const;
    void setSpellingMistakeLineColor(const QColor &col);

    bool showIndentationLines() const;
    void setShowIndentationLines(bool on);

    bool showWholeBracketExpression() const;
    void setShowWholeBracketExpression(bool on);

    bool animateBracketMatching() const;
    void setAnimateBracketMatching(bool on);

    const QColor &templateBackgroundColor() const;
    const QColor &templateEditablePlaceholderColor() const;
    const QColor &templateFocusedEditablePlaceholderColor() const;
    const QColor &templateNotEditablePlaceholderColor() const;

    const QColor &modifiedLineColor() const;
    void setModifiedLineColor(const QColor &col);

    const QColor &savedLineColor() const;
    void setSavedLineColor(const QColor &col);

    const QColor &searchHighlightColor() const;
    void setSearchHighlightColor(const QColor &col);

    const QColor &replaceHighlightColor() const;
    void setReplaceHighlightColor(const QColor &col);

private:
    /**
     * Read the schema properties from the config file.
     */
    void setSchemaInternal(const QString &schema);

    /**
     * Set the font but drop style name before that.
     * Otherwise e.g. styles like bold/italic/... will not work
     */
    void setFontWithDroppedStyleName(const QFont &font);

    QString m_schema;
    QFont m_font;
    QFontMetricsF m_fontMetrics;
    QColor m_backgroundColor;
    QColor m_selectionColor;
    QColor m_highlightedLineColor;
    QColor m_highlightedBracketColor;
    QColor m_wordWrapMarkerColor;
    QColor m_tabMarkerColor;
    QColor m_indentationLineColor;
    QColor m_iconBarColor;
    QColor m_foldingColor;
    QColor m_lineNumberColor;
    QColor m_currentLineNumberColor;
    QColor m_separatorColor;
    QColor m_spellingMistakeLineColor;
    QVector<QColor> m_lineMarkerColor;

    QColor m_templateBackgroundColor;
    QColor m_templateEditablePlaceholderColor;
    QColor m_templateFocusedEditablePlaceholderColor;
    QColor m_templateNotEditablePlaceholderColor;

    QColor m_modifiedLineColor;
    QColor m_savedLineColor;
    QColor m_searchHighlightColor;
    QColor m_replaceHighlightColor;

    bool m_wordWrapMarker = false;
    bool m_showIndentationLines = false;
    bool m_showWholeBracketExpression = false;
    bool m_animateBracketMatching = false;

    bool m_schemaSet : 1;
    bool m_fontSet : 1;
    bool m_wordWrapMarkerSet : 1;
    bool m_showIndentationLinesSet : 1;
    bool m_showWholeBracketExpressionSet : 1;
    bool m_backgroundColorSet : 1;
    bool m_selectionColorSet : 1;
    bool m_highlightedLineColorSet : 1;
    bool m_highlightedBracketColorSet : 1;
    bool m_wordWrapMarkerColorSet : 1;
    bool m_tabMarkerColorSet : 1;
    bool m_indentationLineColorSet : 1;
    bool m_iconBarColorSet : 1;
    bool m_foldingColorSet : 1;
    bool m_lineNumberColorSet : 1;
    bool m_currentLineNumberColorSet : 1;
    bool m_separatorColorSet : 1;
    bool m_spellingMistakeLineColorSet : 1;
    bool m_templateColorsSet : 1;
    bool m_modifiedLineColorSet : 1;
    bool m_savedLineColorSet : 1;
    bool m_searchHighlightColorSet : 1;
    bool m_replaceHighlightColorSet : 1;
    QBitArray m_lineMarkerColorSet;

private:
    static KateRendererConfig *s_global;
    KateRenderer *m_renderer = nullptr;
};

#endif

