/*
    SPDX-FileCopyrightText: 2008 Erlend Hamberg <ehamberg@gmail.com>
    SPDX-FileCopyrightText: 2008 Evgeniy Ivanov <powerfox@kde.ru>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QKeyEvent>
#include <QStringList>
#include <vimode/keyparser.h>

using namespace KateVi;

KeyParser *KeyParser::m_instance = nullptr;

KeyParser::KeyParser()
{
    initKeyTables();
}

KeyParser *KeyParser::self()
{
    if (m_instance == nullptr) {
        m_instance = new KeyParser();
    }

    return m_instance;
}

void KeyParser::initKeyTables()
{
    m_qt2katevi.insert(Qt::Key_Escape, QStringLiteral("esc"));
    m_qt2katevi.insert(Qt::Key_Tab, QStringLiteral("tab"));
    m_qt2katevi.insert(Qt::Key_Backtab, QStringLiteral("backtab"));
    m_qt2katevi.insert(Qt::Key_Backspace, QStringLiteral("backspace"));
    m_qt2katevi.insert(Qt::Key_Return, QStringLiteral("return"));
    m_qt2katevi.insert(Qt::Key_Enter, QStringLiteral("enter"));
    m_qt2katevi.insert(Qt::Key_Insert, QStringLiteral("insert"));
    m_qt2katevi.insert(Qt::Key_Delete, QStringLiteral("delete"));
    m_qt2katevi.insert(Qt::Key_Pause, QStringLiteral("pause"));
    m_qt2katevi.insert(Qt::Key_Print, QStringLiteral("print"));
    m_qt2katevi.insert(Qt::Key_SysReq, QStringLiteral("sysreq"));
    m_qt2katevi.insert(Qt::Key_Clear, QStringLiteral("clear"));
    m_qt2katevi.insert(Qt::Key_Home, QStringLiteral("home"));
    m_qt2katevi.insert(Qt::Key_End, QStringLiteral("end"));
    m_qt2katevi.insert(Qt::Key_Left, QStringLiteral("left"));
    m_qt2katevi.insert(Qt::Key_Up, QStringLiteral("up"));
    m_qt2katevi.insert(Qt::Key_Right, QStringLiteral("right"));
    m_qt2katevi.insert(Qt::Key_Down, QStringLiteral("down"));
    m_qt2katevi.insert(Qt::Key_PageUp, QStringLiteral("pageup"));
    m_qt2katevi.insert(Qt::Key_PageDown, QStringLiteral("pagedown"));
    m_qt2katevi.insert(Qt::Key_Shift, QStringLiteral("shift"));
    m_qt2katevi.insert(Qt::Key_Control, QStringLiteral("control"));
    m_qt2katevi.insert(Qt::Key_Meta, QStringLiteral("meta"));
    m_qt2katevi.insert(Qt::Key_Alt, QStringLiteral("alt"));
    m_qt2katevi.insert(Qt::Key_AltGr, QStringLiteral("altgr"));
    m_qt2katevi.insert(Qt::Key_CapsLock, QStringLiteral("capslock"));
    m_qt2katevi.insert(Qt::Key_NumLock, QStringLiteral("numlock"));
    m_qt2katevi.insert(Qt::Key_ScrollLock, QStringLiteral("scrolllock"));
    m_qt2katevi.insert(Qt::Key_F1, QStringLiteral("f1"));
    m_qt2katevi.insert(Qt::Key_F2, QStringLiteral("f2"));
    m_qt2katevi.insert(Qt::Key_F3, QStringLiteral("f3"));
    m_qt2katevi.insert(Qt::Key_F4, QStringLiteral("f4"));
    m_qt2katevi.insert(Qt::Key_F5, QStringLiteral("f5"));
    m_qt2katevi.insert(Qt::Key_F6, QStringLiteral("f6"));
    m_qt2katevi.insert(Qt::Key_F7, QStringLiteral("f7"));
    m_qt2katevi.insert(Qt::Key_F8, QStringLiteral("f8"));
    m_qt2katevi.insert(Qt::Key_F9, QStringLiteral("f9"));
    m_qt2katevi.insert(Qt::Key_F10, QStringLiteral("f10"));
    m_qt2katevi.insert(Qt::Key_F11, QStringLiteral("f11"));
    m_qt2katevi.insert(Qt::Key_F12, QStringLiteral("f12"));
    m_qt2katevi.insert(Qt::Key_F13, QStringLiteral("f13"));
    m_qt2katevi.insert(Qt::Key_F14, QStringLiteral("f14"));
    m_qt2katevi.insert(Qt::Key_F15, QStringLiteral("f15"));
    m_qt2katevi.insert(Qt::Key_F16, QStringLiteral("f16"));
    m_qt2katevi.insert(Qt::Key_F17, QStringLiteral("f17"));
    m_qt2katevi.insert(Qt::Key_F18, QStringLiteral("f18"));
    m_qt2katevi.insert(Qt::Key_F19, QStringLiteral("f19"));
    m_qt2katevi.insert(Qt::Key_F20, QStringLiteral("f20"));
    m_qt2katevi.insert(Qt::Key_F21, QStringLiteral("f21"));
    m_qt2katevi.insert(Qt::Key_F22, QStringLiteral("f22"));
    m_qt2katevi.insert(Qt::Key_F23, QStringLiteral("f23"));
    m_qt2katevi.insert(Qt::Key_F24, QStringLiteral("f24"));
    m_qt2katevi.insert(Qt::Key_F25, QStringLiteral("f25"));
    m_qt2katevi.insert(Qt::Key_F26, QStringLiteral("f26"));
    m_qt2katevi.insert(Qt::Key_F27, QStringLiteral("f27"));
    m_qt2katevi.insert(Qt::Key_F28, QStringLiteral("f28"));
    m_qt2katevi.insert(Qt::Key_F29, QStringLiteral("f29"));
    m_qt2katevi.insert(Qt::Key_F30, QStringLiteral("f30"));
    m_qt2katevi.insert(Qt::Key_F31, QStringLiteral("f31"));
    m_qt2katevi.insert(Qt::Key_F32, QStringLiteral("f32"));
    m_qt2katevi.insert(Qt::Key_F33, QStringLiteral("f33"));
    m_qt2katevi.insert(Qt::Key_F34, QStringLiteral("f34"));
    m_qt2katevi.insert(Qt::Key_F35, QStringLiteral("f35"));
    m_qt2katevi.insert(Qt::Key_Super_L, QStringLiteral("super_l"));
    m_qt2katevi.insert(Qt::Key_Super_R, QStringLiteral("super_r"));
    m_qt2katevi.insert(Qt::Key_Menu, QStringLiteral("menu"));
    m_qt2katevi.insert(Qt::Key_Hyper_L, QStringLiteral("hyper_l"));
    m_qt2katevi.insert(Qt::Key_Hyper_R, QStringLiteral("hyper_r"));
    m_qt2katevi.insert(Qt::Key_Help, QStringLiteral("help"));
    m_qt2katevi.insert(Qt::Key_Direction_L, QStringLiteral("direction_l"));
    m_qt2katevi.insert(Qt::Key_Direction_R, QStringLiteral("direction_r"));
    m_qt2katevi.insert(Qt::Key_Multi_key, QStringLiteral("multi_key"));
    m_qt2katevi.insert(Qt::Key_Codeinput, QStringLiteral("codeinput"));
    m_qt2katevi.insert(Qt::Key_SingleCandidate, QStringLiteral("singlecandidate"));
    m_qt2katevi.insert(Qt::Key_MultipleCandidate, QStringLiteral("multiplecandidate"));
    m_qt2katevi.insert(Qt::Key_PreviousCandidate, QStringLiteral("previouscandidate"));
    m_qt2katevi.insert(Qt::Key_Mode_switch, QStringLiteral("mode_switch"));
    m_qt2katevi.insert(Qt::Key_Kanji, QStringLiteral("kanji"));
    m_qt2katevi.insert(Qt::Key_Muhenkan, QStringLiteral("muhenkan"));
    m_qt2katevi.insert(Qt::Key_Henkan, QStringLiteral("henkan"));
    m_qt2katevi.insert(Qt::Key_Romaji, QStringLiteral("romaji"));
    m_qt2katevi.insert(Qt::Key_Hiragana, QStringLiteral("hiragana"));
    m_qt2katevi.insert(Qt::Key_Katakana, QStringLiteral("katakana"));
    m_qt2katevi.insert(Qt::Key_Hiragana_Katakana, QStringLiteral("hiragana_katakana"));
    m_qt2katevi.insert(Qt::Key_Zenkaku, QStringLiteral("zenkaku"));
    m_qt2katevi.insert(Qt::Key_Hankaku, QStringLiteral("hankaku"));
    m_qt2katevi.insert(Qt::Key_Zenkaku_Hankaku, QStringLiteral("zenkaku_hankaku"));
    m_qt2katevi.insert(Qt::Key_Touroku, QStringLiteral("touroku"));
    m_qt2katevi.insert(Qt::Key_Massyo, QStringLiteral("massyo"));
    m_qt2katevi.insert(Qt::Key_Kana_Lock, QStringLiteral("kana_lock"));
    m_qt2katevi.insert(Qt::Key_Kana_Shift, QStringLiteral("kana_shift"));
    m_qt2katevi.insert(Qt::Key_Eisu_Shift, QStringLiteral("eisu_shift"));
    m_qt2katevi.insert(Qt::Key_Eisu_toggle, QStringLiteral("eisu_toggle"));
    m_qt2katevi.insert(Qt::Key_Hangul, QStringLiteral("hangul"));
    m_qt2katevi.insert(Qt::Key_Hangul_Start, QStringLiteral("hangul_start"));
    m_qt2katevi.insert(Qt::Key_Hangul_End, QStringLiteral("hangul_end"));
    m_qt2katevi.insert(Qt::Key_Hangul_Hanja, QStringLiteral("hangul_hanja"));
    m_qt2katevi.insert(Qt::Key_Hangul_Jamo, QStringLiteral("hangul_jamo"));
    m_qt2katevi.insert(Qt::Key_Hangul_Romaja, QStringLiteral("hangul_romaja"));
    m_qt2katevi.insert(Qt::Key_Hangul_Jeonja, QStringLiteral("hangul_jeonja"));
    m_qt2katevi.insert(Qt::Key_Hangul_Banja, QStringLiteral("hangul_banja"));
    m_qt2katevi.insert(Qt::Key_Hangul_PreHanja, QStringLiteral("hangul_prehanja"));
    m_qt2katevi.insert(Qt::Key_Hangul_PostHanja, QStringLiteral("hangul_posthanja"));
    m_qt2katevi.insert(Qt::Key_Hangul_Special, QStringLiteral("hangul_special"));
    m_qt2katevi.insert(Qt::Key_Dead_Grave, QStringLiteral("dead_grave"));
    m_qt2katevi.insert(Qt::Key_Dead_Acute, QStringLiteral("dead_acute"));
    m_qt2katevi.insert(Qt::Key_Dead_Circumflex, QStringLiteral("dead_circumflex"));
    m_qt2katevi.insert(Qt::Key_Dead_Tilde, QStringLiteral("dead_tilde"));
    m_qt2katevi.insert(Qt::Key_Dead_Macron, QStringLiteral("dead_macron"));
    m_qt2katevi.insert(Qt::Key_Dead_Breve, QStringLiteral("dead_breve"));
    m_qt2katevi.insert(Qt::Key_Dead_Abovedot, QStringLiteral("dead_abovedot"));
    m_qt2katevi.insert(Qt::Key_Dead_Diaeresis, QStringLiteral("dead_diaeresis"));
    m_qt2katevi.insert(Qt::Key_Dead_Abovering, QStringLiteral("dead_abovering"));
    m_qt2katevi.insert(Qt::Key_Dead_Doubleacute, QStringLiteral("dead_doubleacute"));
    m_qt2katevi.insert(Qt::Key_Dead_Caron, QStringLiteral("dead_caron"));
    m_qt2katevi.insert(Qt::Key_Dead_Cedilla, QStringLiteral("dead_cedilla"));
    m_qt2katevi.insert(Qt::Key_Dead_Ogonek, QStringLiteral("dead_ogonek"));
    m_qt2katevi.insert(Qt::Key_Dead_Iota, QStringLiteral("dead_iota"));
    m_qt2katevi.insert(Qt::Key_Dead_Voiced_Sound, QStringLiteral("dead_voiced_sound"));
    m_qt2katevi.insert(Qt::Key_Dead_Semivoiced_Sound, QStringLiteral("dead_semivoiced_sound"));
    m_qt2katevi.insert(Qt::Key_Dead_Belowdot, QStringLiteral("dead_belowdot"));
    m_qt2katevi.insert(Qt::Key_Dead_Hook, QStringLiteral("dead_hook"));
    m_qt2katevi.insert(Qt::Key_Dead_Horn, QStringLiteral("dead_horn"));
    m_qt2katevi.insert(Qt::Key_Back, QStringLiteral("back"));
    m_qt2katevi.insert(Qt::Key_Forward, QStringLiteral("forward"));
    m_qt2katevi.insert(Qt::Key_Stop, QStringLiteral("stop"));
    m_qt2katevi.insert(Qt::Key_Refresh, QStringLiteral("refresh"));
    m_qt2katevi.insert(Qt::Key_VolumeDown, QStringLiteral("volumedown"));
    m_qt2katevi.insert(Qt::Key_VolumeMute, QStringLiteral("volumemute"));
    m_qt2katevi.insert(Qt::Key_VolumeUp, QStringLiteral("volumeup"));
    m_qt2katevi.insert(Qt::Key_BassBoost, QStringLiteral("bassboost"));
    m_qt2katevi.insert(Qt::Key_BassUp, QStringLiteral("bassup"));
    m_qt2katevi.insert(Qt::Key_BassDown, QStringLiteral("bassdown"));
    m_qt2katevi.insert(Qt::Key_TrebleUp, QStringLiteral("trebleup"));
    m_qt2katevi.insert(Qt::Key_TrebleDown, QStringLiteral("trebledown"));
    m_qt2katevi.insert(Qt::Key_MediaPlay, QStringLiteral("mediaplay"));
    m_qt2katevi.insert(Qt::Key_MediaStop, QStringLiteral("mediastop"));
    m_qt2katevi.insert(Qt::Key_MediaPrevious, QStringLiteral("mediaprevious"));
    m_qt2katevi.insert(Qt::Key_MediaNext, QStringLiteral("medianext"));
    m_qt2katevi.insert(Qt::Key_MediaRecord, QStringLiteral("mediarecord"));
    m_qt2katevi.insert(Qt::Key_HomePage, QStringLiteral("homepage"));
    m_qt2katevi.insert(Qt::Key_Favorites, QStringLiteral("favorites"));
    m_qt2katevi.insert(Qt::Key_Search, QStringLiteral("search"));
    m_qt2katevi.insert(Qt::Key_Standby, QStringLiteral("standby"));
    m_qt2katevi.insert(Qt::Key_OpenUrl, QStringLiteral("openurl"));
    m_qt2katevi.insert(Qt::Key_LaunchMail, QStringLiteral("launchmail"));
    m_qt2katevi.insert(Qt::Key_LaunchMedia, QStringLiteral("launchmedia"));
    m_qt2katevi.insert(Qt::Key_Launch0, QStringLiteral("launch0"));
    m_qt2katevi.insert(Qt::Key_Launch1, QStringLiteral("launch1"));
    m_qt2katevi.insert(Qt::Key_Launch2, QStringLiteral("launch2"));
    m_qt2katevi.insert(Qt::Key_Launch3, QStringLiteral("launch3"));
    m_qt2katevi.insert(Qt::Key_Launch4, QStringLiteral("launch4"));
    m_qt2katevi.insert(Qt::Key_Launch5, QStringLiteral("launch5"));
    m_qt2katevi.insert(Qt::Key_Launch6, QStringLiteral("launch6"));
    m_qt2katevi.insert(Qt::Key_Launch7, QStringLiteral("launch7"));
    m_qt2katevi.insert(Qt::Key_Launch8, QStringLiteral("launch8"));
    m_qt2katevi.insert(Qt::Key_Launch9, QStringLiteral("launch9"));
    m_qt2katevi.insert(Qt::Key_LaunchA, QStringLiteral("launcha"));
    m_qt2katevi.insert(Qt::Key_LaunchB, QStringLiteral("launchb"));
    m_qt2katevi.insert(Qt::Key_LaunchC, QStringLiteral("launchc"));
    m_qt2katevi.insert(Qt::Key_LaunchD, QStringLiteral("launchd"));
    m_qt2katevi.insert(Qt::Key_LaunchE, QStringLiteral("launche"));
    m_qt2katevi.insert(Qt::Key_LaunchF, QStringLiteral("launchf"));
    m_qt2katevi.insert(Qt::Key_MediaLast, QStringLiteral("medialast"));
    m_qt2katevi.insert(Qt::Key_unknown, QStringLiteral("unknown"));
    m_qt2katevi.insert(Qt::Key_Call, QStringLiteral("call"));
    m_qt2katevi.insert(Qt::Key_Context1, QStringLiteral("context1"));
    m_qt2katevi.insert(Qt::Key_Context2, QStringLiteral("context2"));
    m_qt2katevi.insert(Qt::Key_Context3, QStringLiteral("context3"));
    m_qt2katevi.insert(Qt::Key_Context4, QStringLiteral("context4"));
    m_qt2katevi.insert(Qt::Key_Flip, QStringLiteral("flip"));
    m_qt2katevi.insert(Qt::Key_Hangup, QStringLiteral("hangup"));
    m_qt2katevi.insert(Qt::Key_No, QStringLiteral("no"));
    m_qt2katevi.insert(Qt::Key_Select, QStringLiteral("select"));
    m_qt2katevi.insert(Qt::Key_Yes, QStringLiteral("yes"));
    m_qt2katevi.insert(Qt::Key_Execute, QStringLiteral("execute"));
    m_qt2katevi.insert(Qt::Key_Printer, QStringLiteral("printer"));
    m_qt2katevi.insert(Qt::Key_Play, QStringLiteral("play"));
    m_qt2katevi.insert(Qt::Key_Sleep, QStringLiteral("sleep"));
    m_qt2katevi.insert(Qt::Key_Zoom, QStringLiteral("zoom"));
    m_qt2katevi.insert(Qt::Key_Cancel, QStringLiteral("cancel"));

    for (QHash<int, QString>::const_iterator i = m_qt2katevi.constBegin(); i != m_qt2katevi.constEnd(); ++i) {
        m_katevi2qt.insert(i.value(), i.key());
    }
    m_katevi2qt.insert(QStringLiteral("cr"), Qt::Key_Enter);

    m_nameToKeyCode.insert(QStringLiteral("invalid"), -1);
    m_nameToKeyCode.insert(QStringLiteral("esc"), 1);
    m_nameToKeyCode.insert(QStringLiteral("tab"), 2);
    m_nameToKeyCode.insert(QStringLiteral("backtab"), 3);
    m_nameToKeyCode.insert(QStringLiteral("backspace"), 4);
    m_nameToKeyCode.insert(QStringLiteral("return"), 5);
    m_nameToKeyCode.insert(QStringLiteral("enter"), 6);
    m_nameToKeyCode.insert(QStringLiteral("insert"), 7);
    m_nameToKeyCode.insert(QStringLiteral("delete"), 8);
    m_nameToKeyCode.insert(QStringLiteral("pause"), 9);
    m_nameToKeyCode.insert(QStringLiteral("print"), 10);
    m_nameToKeyCode.insert(QStringLiteral("sysreq"), 11);
    m_nameToKeyCode.insert(QStringLiteral("clear"), 12);
    m_nameToKeyCode.insert(QStringLiteral("home"), 13);
    m_nameToKeyCode.insert(QStringLiteral("end"), 14);
    m_nameToKeyCode.insert(QStringLiteral("left"), 15);
    m_nameToKeyCode.insert(QStringLiteral("up"), 16);
    m_nameToKeyCode.insert(QStringLiteral("right"), 17);
    m_nameToKeyCode.insert(QStringLiteral("down"), 18);
    m_nameToKeyCode.insert(QStringLiteral("pageup"), 19);
    m_nameToKeyCode.insert(QStringLiteral("pagedown"), 20);
    m_nameToKeyCode.insert(QStringLiteral("shift"), 21);
    m_nameToKeyCode.insert(QStringLiteral("control"), 22);
    m_nameToKeyCode.insert(QStringLiteral("meta"), 23);
    m_nameToKeyCode.insert(QStringLiteral("alt"), 24);
    m_nameToKeyCode.insert(QStringLiteral("altgr"), 25);
    m_nameToKeyCode.insert(QStringLiteral("capslock"), 26);
    m_nameToKeyCode.insert(QStringLiteral("numlock"), 27);
    m_nameToKeyCode.insert(QStringLiteral("scrolllock"), 28);
    m_nameToKeyCode.insert(QStringLiteral("f1"), 29);
    m_nameToKeyCode.insert(QStringLiteral("f2"), 30);
    m_nameToKeyCode.insert(QStringLiteral("f3"), 31);
    m_nameToKeyCode.insert(QStringLiteral("f4"), 32);
    m_nameToKeyCode.insert(QStringLiteral("f5"), 33);
    m_nameToKeyCode.insert(QStringLiteral("f6"), 34);
    m_nameToKeyCode.insert(QStringLiteral("f7"), 35);
    m_nameToKeyCode.insert(QStringLiteral("f8"), 36);
    m_nameToKeyCode.insert(QStringLiteral("f9"), 37);
    m_nameToKeyCode.insert(QStringLiteral("f10"), 38);
    m_nameToKeyCode.insert(QStringLiteral("f11"), 39);
    m_nameToKeyCode.insert(QStringLiteral("f12"), 40);
    m_nameToKeyCode.insert(QStringLiteral("f13"), 41);
    m_nameToKeyCode.insert(QStringLiteral("f14"), 42);
    m_nameToKeyCode.insert(QStringLiteral("f15"), 43);
    m_nameToKeyCode.insert(QStringLiteral("f16"), 44);
    m_nameToKeyCode.insert(QStringLiteral("f17"), 45);
    m_nameToKeyCode.insert(QStringLiteral("f18"), 46);
    m_nameToKeyCode.insert(QStringLiteral("f19"), 47);
    m_nameToKeyCode.insert(QStringLiteral("f20"), 48);
    m_nameToKeyCode.insert(QStringLiteral("f21"), 49);
    m_nameToKeyCode.insert(QStringLiteral("f22"), 50);
    m_nameToKeyCode.insert(QStringLiteral("f23"), 51);
    m_nameToKeyCode.insert(QStringLiteral("f24"), 52);
    m_nameToKeyCode.insert(QStringLiteral("f25"), 53);
    m_nameToKeyCode.insert(QStringLiteral("f26"), 54);
    m_nameToKeyCode.insert(QStringLiteral("f27"), 55);
    m_nameToKeyCode.insert(QStringLiteral("f28"), 56);
    m_nameToKeyCode.insert(QStringLiteral("f29"), 57);
    m_nameToKeyCode.insert(QStringLiteral("f30"), 58);
    m_nameToKeyCode.insert(QStringLiteral("f31"), 59);
    m_nameToKeyCode.insert(QStringLiteral("f32"), 60);
    m_nameToKeyCode.insert(QStringLiteral("f33"), 61);
    m_nameToKeyCode.insert(QStringLiteral("f34"), 62);
    m_nameToKeyCode.insert(QStringLiteral("f35"), 63);
    m_nameToKeyCode.insert(QStringLiteral("super_l"), 64);
    m_nameToKeyCode.insert(QStringLiteral("super_r"), 65);
    m_nameToKeyCode.insert(QStringLiteral("menu"), 66);
    m_nameToKeyCode.insert(QStringLiteral("hyper_l"), 67);
    m_nameToKeyCode.insert(QStringLiteral("hyper_r"), 68);
    m_nameToKeyCode.insert(QStringLiteral("help"), 69);
    m_nameToKeyCode.insert(QStringLiteral("direction_l"), 70);
    m_nameToKeyCode.insert(QStringLiteral("direction_r"), 71);
    m_nameToKeyCode.insert(QStringLiteral("multi_key"), 172);
    m_nameToKeyCode.insert(QStringLiteral("codeinput"), 173);
    m_nameToKeyCode.insert(QStringLiteral("singlecandidate"), 174);
    m_nameToKeyCode.insert(QStringLiteral("multiplecandidate"), 175);
    m_nameToKeyCode.insert(QStringLiteral("previouscandidate"), 176);
    m_nameToKeyCode.insert(QStringLiteral("mode_switch"), 177);
    m_nameToKeyCode.insert(QStringLiteral("kanji"), 178);
    m_nameToKeyCode.insert(QStringLiteral("muhenkan"), 179);
    m_nameToKeyCode.insert(QStringLiteral("henkan"), 180);
    m_nameToKeyCode.insert(QStringLiteral("romaji"), 181);
    m_nameToKeyCode.insert(QStringLiteral("hiragana"), 182);
    m_nameToKeyCode.insert(QStringLiteral("katakana"), 183);
    m_nameToKeyCode.insert(QStringLiteral("hiragana_katakana"), 184);
    m_nameToKeyCode.insert(QStringLiteral("zenkaku"), 185);
    m_nameToKeyCode.insert(QStringLiteral("hankaku"), 186);
    m_nameToKeyCode.insert(QStringLiteral("zenkaku_hankaku"), 187);
    m_nameToKeyCode.insert(QStringLiteral("touroku"), 188);
    m_nameToKeyCode.insert(QStringLiteral("massyo"), 189);
    m_nameToKeyCode.insert(QStringLiteral("kana_lock"), 190);
    m_nameToKeyCode.insert(QStringLiteral("kana_shift"), 191);
    m_nameToKeyCode.insert(QStringLiteral("eisu_shift"), 192);
    m_nameToKeyCode.insert(QStringLiteral("eisu_toggle"), 193);
    m_nameToKeyCode.insert(QStringLiteral("hangul"), 194);
    m_nameToKeyCode.insert(QStringLiteral("hangul_start"), 195);
    m_nameToKeyCode.insert(QStringLiteral("hangul_end"), 196);
    m_nameToKeyCode.insert(QStringLiteral("hangul_hanja"), 197);
    m_nameToKeyCode.insert(QStringLiteral("hangul_jamo"), 198);
    m_nameToKeyCode.insert(QStringLiteral("hangul_romaja"), 199);
    m_nameToKeyCode.insert(QStringLiteral("hangul_jeonja"), 200);
    m_nameToKeyCode.insert(QStringLiteral("hangul_banja"), 201);
    m_nameToKeyCode.insert(QStringLiteral("hangul_prehanja"), 202);
    m_nameToKeyCode.insert(QStringLiteral("hangul_posthanja"), 203);
    m_nameToKeyCode.insert(QStringLiteral("hangul_special"), 204);
    m_nameToKeyCode.insert(QStringLiteral("dead_grave"), 205);
    m_nameToKeyCode.insert(QStringLiteral("dead_acute"), 206);
    m_nameToKeyCode.insert(QStringLiteral("dead_circumflex"), 207);
    m_nameToKeyCode.insert(QStringLiteral("dead_tilde"), 208);
    m_nameToKeyCode.insert(QStringLiteral("dead_macron"), 209);
    m_nameToKeyCode.insert(QStringLiteral("dead_breve"), 210);
    m_nameToKeyCode.insert(QStringLiteral("dead_abovedot"), 211);
    m_nameToKeyCode.insert(QStringLiteral("dead_diaeresis"), 212);
    m_nameToKeyCode.insert(QStringLiteral("dead_abovering"), 213);
    m_nameToKeyCode.insert(QStringLiteral("dead_doubleacute"), 214);
    m_nameToKeyCode.insert(QStringLiteral("dead_caron"), 215);
    m_nameToKeyCode.insert(QStringLiteral("dead_cedilla"), 216);
    m_nameToKeyCode.insert(QStringLiteral("dead_ogonek"), 217);
    m_nameToKeyCode.insert(QStringLiteral("dead_iota"), 218);
    m_nameToKeyCode.insert(QStringLiteral("dead_voiced_sound"), 219);
    m_nameToKeyCode.insert(QStringLiteral("dead_semivoiced_sound"), 220);
    m_nameToKeyCode.insert(QStringLiteral("dead_belowdot"), 221);
    m_nameToKeyCode.insert(QStringLiteral("dead_hook"), 222);
    m_nameToKeyCode.insert(QStringLiteral("dead_horn"), 223);
    m_nameToKeyCode.insert(QStringLiteral("back"), 224);
    m_nameToKeyCode.insert(QStringLiteral("forward"), 225);
    m_nameToKeyCode.insert(QStringLiteral("stop"), 226);
    m_nameToKeyCode.insert(QStringLiteral("refresh"), 227);
    m_nameToKeyCode.insert(QStringLiteral("volumedown"), 228);
    m_nameToKeyCode.insert(QStringLiteral("volumemute"), 229);
    m_nameToKeyCode.insert(QStringLiteral("volumeup"), 230);
    m_nameToKeyCode.insert(QStringLiteral("bassboost"), 231);
    m_nameToKeyCode.insert(QStringLiteral("bassup"), 232);
    m_nameToKeyCode.insert(QStringLiteral("bassdown"), 233);
    m_nameToKeyCode.insert(QStringLiteral("trebleup"), 234);
    m_nameToKeyCode.insert(QStringLiteral("trebledown"), 235);
    m_nameToKeyCode.insert(QStringLiteral("mediaplay"), 236);
    m_nameToKeyCode.insert(QStringLiteral("mediastop"), 237);
    m_nameToKeyCode.insert(QStringLiteral("mediaprevious"), 238);
    m_nameToKeyCode.insert(QStringLiteral("medianext"), 239);
    m_nameToKeyCode.insert(QStringLiteral("mediarecord"), 240);
    m_nameToKeyCode.insert(QStringLiteral("homepage"), 241);
    m_nameToKeyCode.insert(QStringLiteral("favorites"), 242);
    m_nameToKeyCode.insert(QStringLiteral("search"), 243);
    m_nameToKeyCode.insert(QStringLiteral("standby"), 244);
    m_nameToKeyCode.insert(QStringLiteral("openurl"), 245);
    m_nameToKeyCode.insert(QStringLiteral("launchmail"), 246);
    m_nameToKeyCode.insert(QStringLiteral("launchmedia"), 247);
    m_nameToKeyCode.insert(QStringLiteral("launch0"), 248);
    m_nameToKeyCode.insert(QStringLiteral("launch1"), 249);
    m_nameToKeyCode.insert(QStringLiteral("launch2"), 250);
    m_nameToKeyCode.insert(QStringLiteral("launch3"), 251);
    m_nameToKeyCode.insert(QStringLiteral("launch4"), 252);
    m_nameToKeyCode.insert(QStringLiteral("launch5"), 253);
    m_nameToKeyCode.insert(QStringLiteral("launch6"), 254);
    m_nameToKeyCode.insert(QStringLiteral("launch7"), 255);
    m_nameToKeyCode.insert(QStringLiteral("launch8"), 256);
    m_nameToKeyCode.insert(QStringLiteral("launch9"), 257);
    m_nameToKeyCode.insert(QStringLiteral("launcha"), 258);
    m_nameToKeyCode.insert(QStringLiteral("launchb"), 259);
    m_nameToKeyCode.insert(QStringLiteral("launchc"), 260);
    m_nameToKeyCode.insert(QStringLiteral("launchd"), 261);
    m_nameToKeyCode.insert(QStringLiteral("launche"), 262);
    m_nameToKeyCode.insert(QStringLiteral("launchf"), 263);
    m_nameToKeyCode.insert(QStringLiteral("medialast"), 264);
    m_nameToKeyCode.insert(QStringLiteral("unknown"), 265);
    m_nameToKeyCode.insert(QStringLiteral("call"), 266);
    m_nameToKeyCode.insert(QStringLiteral("context1"), 267);
    m_nameToKeyCode.insert(QStringLiteral("context2"), 268);
    m_nameToKeyCode.insert(QStringLiteral("context3"), 269);
    m_nameToKeyCode.insert(QStringLiteral("context4"), 270);
    m_nameToKeyCode.insert(QStringLiteral("flip"), 271);
    m_nameToKeyCode.insert(QStringLiteral("hangup"), 272);
    m_nameToKeyCode.insert(QStringLiteral("no"), 273);
    m_nameToKeyCode.insert(QStringLiteral("select"), 274);
    m_nameToKeyCode.insert(QStringLiteral("yes"), 275);
    m_nameToKeyCode.insert(QStringLiteral("execute"), 276);
    m_nameToKeyCode.insert(QStringLiteral("printer"), 277);
    m_nameToKeyCode.insert(QStringLiteral("play"), 278);
    m_nameToKeyCode.insert(QStringLiteral("sleep"), 279);
    m_nameToKeyCode.insert(QStringLiteral("zoom"), 280);
    m_nameToKeyCode.insert(QStringLiteral("cancel"), 281);

    m_nameToKeyCode.insert(QStringLiteral("a"), 282);
    m_nameToKeyCode.insert(QStringLiteral("b"), 283);
    m_nameToKeyCode.insert(QStringLiteral("c"), 284);
    m_nameToKeyCode.insert(QStringLiteral("d"), 285);
    m_nameToKeyCode.insert(QStringLiteral("e"), 286);
    m_nameToKeyCode.insert(QStringLiteral("f"), 287);
    m_nameToKeyCode.insert(QStringLiteral("g"), 288);
    m_nameToKeyCode.insert(QStringLiteral("h"), 289);
    m_nameToKeyCode.insert(QStringLiteral("i"), 290);
    m_nameToKeyCode.insert(QStringLiteral("j"), 291);
    m_nameToKeyCode.insert(QStringLiteral("k"), 292);
    m_nameToKeyCode.insert(QStringLiteral("l"), 293);
    m_nameToKeyCode.insert(QStringLiteral("m"), 294);
    m_nameToKeyCode.insert(QStringLiteral("n"), 295);
    m_nameToKeyCode.insert(QStringLiteral("o"), 296);
    m_nameToKeyCode.insert(QStringLiteral("p"), 297);
    m_nameToKeyCode.insert(QStringLiteral("q"), 298);
    m_nameToKeyCode.insert(QStringLiteral("r"), 299);
    m_nameToKeyCode.insert(QStringLiteral("s"), 300);
    m_nameToKeyCode.insert(QStringLiteral("t"), 301);
    m_nameToKeyCode.insert(QStringLiteral("u"), 302);
    m_nameToKeyCode.insert(QStringLiteral("v"), 303);
    m_nameToKeyCode.insert(QStringLiteral("w"), 304);
    m_nameToKeyCode.insert(QStringLiteral("x"), 305);
    m_nameToKeyCode.insert(QStringLiteral("y"), 306);
    m_nameToKeyCode.insert(QStringLiteral("z"), 307);
    m_nameToKeyCode.insert(QStringLiteral("`"), 308);
    m_nameToKeyCode.insert(QStringLiteral("!"), 309);
    m_nameToKeyCode.insert(QStringLiteral("\""), 310);
    m_nameToKeyCode.insert(QStringLiteral("$"), 311);
    m_nameToKeyCode.insert(QStringLiteral("%"), 312);
    m_nameToKeyCode.insert(QStringLiteral("^"), 313);
    m_nameToKeyCode.insert(QStringLiteral("&"), 314);
    m_nameToKeyCode.insert(QStringLiteral("*"), 315);
    m_nameToKeyCode.insert(QStringLiteral("("), 316);
    m_nameToKeyCode.insert(QStringLiteral(")"), 317);
    m_nameToKeyCode.insert(QStringLiteral("-"), 318);
    m_nameToKeyCode.insert(QStringLiteral("_"), 319);
    m_nameToKeyCode.insert(QStringLiteral("="), 320);
    m_nameToKeyCode.insert(QStringLiteral("+"), 321);
    m_nameToKeyCode.insert(QStringLiteral("["), 322);
    m_nameToKeyCode.insert(QStringLiteral("]"), 323);
    m_nameToKeyCode.insert(QStringLiteral("{"), 324);
    m_nameToKeyCode.insert(QStringLiteral("}"), 325);
    m_nameToKeyCode.insert(QStringLiteral(":"), 326);
    m_nameToKeyCode.insert(QStringLiteral(";"), 327);
    m_nameToKeyCode.insert(QStringLiteral("@"), 328);
    m_nameToKeyCode.insert(QStringLiteral("'"), 329);
    m_nameToKeyCode.insert(QStringLiteral("#"), 330);
    m_nameToKeyCode.insert(QStringLiteral("~"), 331);
    m_nameToKeyCode.insert(QStringLiteral("\\"), 332);
    m_nameToKeyCode.insert(QStringLiteral("|"), 333);
    m_nameToKeyCode.insert(QStringLiteral(","), 334);
    m_nameToKeyCode.insert(QStringLiteral("."), 335);
    // m_nameToKeyCode.insert( QLatin1String( ">" ), 336 );
    m_nameToKeyCode.insert(QStringLiteral("/"), 337);
    m_nameToKeyCode.insert(QStringLiteral("?"), 338);
    m_nameToKeyCode.insert(QStringLiteral(" "), 339);
    // m_nameToKeyCode.insert( QLatin1String( "<" ), 341 );
    m_nameToKeyCode.insert(QStringLiteral("0"), 340);
    m_nameToKeyCode.insert(QStringLiteral("1"), 341);
    m_nameToKeyCode.insert(QStringLiteral("2"), 342);
    m_nameToKeyCode.insert(QStringLiteral("3"), 343);
    m_nameToKeyCode.insert(QStringLiteral("4"), 344);
    m_nameToKeyCode.insert(QStringLiteral("5"), 345);
    m_nameToKeyCode.insert(QStringLiteral("6"), 346);
    m_nameToKeyCode.insert(QStringLiteral("7"), 347);
    m_nameToKeyCode.insert(QStringLiteral("8"), 348);
    m_nameToKeyCode.insert(QStringLiteral("9"), 349);
    m_nameToKeyCode.insert(QStringLiteral("cr"), 350);
    m_nameToKeyCode.insert(QStringLiteral("leader"), 351);
    m_nameToKeyCode.insert(QStringLiteral("nop"), 352);

    for (QHash<QString, int>::const_iterator i = m_nameToKeyCode.constBegin(); i != m_nameToKeyCode.constEnd(); ++i) {
        m_keyCodeToName.insert(i.value(), i.key());
    }
}

QString KeyParser::qt2vi(int key) const
{
    return (m_qt2katevi.contains(key) ? m_qt2katevi.value(key) : QStringLiteral("invalid"));
}

int KeyParser::vi2qt(const QString &keypress) const
{
    return (m_katevi2qt.contains(keypress) ? m_katevi2qt.value(keypress) : -1);
}

int KeyParser::encoded2qt(const QString &keypress) const
{
    QString key = KeyParser::self()->decodeKeySequence(keypress);

    if (key.length() > 2 && key[0] == QLatin1Char('<') && key[key.length() - 1] == QLatin1Char('>')) {
        key = key.mid(1, key.length() - 2);
    }
    return (m_katevi2qt.contains(key) ? m_katevi2qt.value(key) : -1);
}

const QString KeyParser::encodeKeySequence(const QString &keys) const
{
    QString encodedSequence;
    unsigned int keyCodeTemp = 0;

    const QStringView keysView(keys);
    bool insideTag = false;
    QChar c;
    for (int i = 0; i < keys.length(); i++) {
        c = keys.at(i);
        if (insideTag) {
            if (c == QLatin1Char('>')) {
                QChar code(0xE000 + keyCodeTemp);
                encodedSequence.append(code);
                keyCodeTemp = 0;
                insideTag = false;
                continue;
            } else {
                // contains modifiers
                if (keysView.mid(i).indexOf(QLatin1Char('-')) != -1 && keysView.mid(i).indexOf(QLatin1Char('-')) < keysView.mid(i).indexOf(QLatin1Char('>'))) {
                    // Perform something similar to a split on '-', except that we want to keep the occurrences of '-'
                    // e.g. <c-s-a> will equate to the list of tokens "c-", "s-", "a".
                    // A straight split on '-' would give us "c", "s", "a", in which case we lose the piece of information that
                    // 'a' is just the 'a' key, not the 'alt' modifier.
                    const QString untilClosing = keys.mid(i, keysView.mid(i).indexOf(QLatin1Char('>'))).toLower();
                    QStringList tokens;
                    int currentPos = 0;
                    int nextHypen = -1;
                    while ((nextHypen = untilClosing.indexOf(QLatin1Char('-'), currentPos)) != -1) {
                        tokens << untilClosing.mid(currentPos, nextHypen - currentPos + 1);
                        currentPos = nextHypen + 1;
                    }
                    tokens << untilClosing.mid(currentPos);

                    for (const QString &str : std::as_const(tokens)) {
                        if (str == QLatin1String("s-") && (keyCodeTemp & 0x01) != 0x1) {
                            keyCodeTemp += 0x1;
                        } else if (str == QLatin1String("c-") && (keyCodeTemp & 0x02) != 0x2) {
                            keyCodeTemp += 0x2;
                        } else if (str == QLatin1String("a-") && (keyCodeTemp & 0x04) != 0x4) {
                            keyCodeTemp += 0x4;
                        } else if (str == QLatin1String("m-") && (keyCodeTemp & 0x08) != 0x8) {
                            keyCodeTemp += 0x8;
                        } else {
                            if (m_nameToKeyCode.contains(str) || (str.length() == 1 && str.at(0).isLetterOrNumber())) {
                                if (m_nameToKeyCode.contains(str)) {
                                    keyCodeTemp += m_nameToKeyCode.value(str) * 0x10;
                                } else {
                                    keyCodeTemp += str.at(0).unicode() * 0x10;
                                }
                            } else {
                                int endOfBlock = keys.indexOf(QLatin1Char('>'));
                                if (endOfBlock -= -1) {
                                    endOfBlock = keys.length() - 1;
                                }
                                encodedSequence.clear();
                                encodedSequence.append(m_nameToKeyCode.value(QStringLiteral("invalid")));
                                break;
                            }
                        }
                    }
                } else {
                    QString str = keys.mid(i, keys.indexOf(QLatin1Char('>'), i) - i).toLower();
                    if (keys.indexOf(QLatin1Char('>'), i) != -1 && (m_nameToKeyCode.contains(str) || (str.length() == 1 && str.at(0).isLetterOrNumber()))) {
                        if (m_nameToKeyCode.contains(str)) {
                            keyCodeTemp += m_nameToKeyCode.value(str) * 0x10;
                        } else {
                            keyCodeTemp += str.at(0).unicode() * 0x10;
                        }
                    } else {
                        int endOfBlock = keys.indexOf(QLatin1Char('>'));
                        if (endOfBlock -= -1) {
                            endOfBlock = keys.length() - 1;
                        }
                        encodedSequence.clear();
                        keyCodeTemp = m_nameToKeyCode.value(QStringLiteral("invalid")) * 0x10;
                    }
                }
                i += keysView.mid(i, keysView.mid(i).indexOf(QLatin1Char('>'))).length() - 1;
            }
        } else {
            if (c == QLatin1Char('<')) {
                // If there's no closing '>', or if there is an opening '<' before the next '>', interpret as a literal '<'
                // If we are <space>, encode as a literal " ".
                const QStringView rest = keysView.mid(i);
                if (rest.indexOf(QLatin1Char('>'), 1) != -1 && rest.mid(1, rest.indexOf(QLatin1Char('>'), 1) - 1) == QLatin1String("space")) {
                    encodedSequence.append(QLatin1Char(' '));
                    i += rest.indexOf(QLatin1Char('>'), 1);
                    continue;
                } else if (rest.indexOf(QLatin1Char('>'), 1) == -1
                           || (rest.indexOf(QLatin1Char('<'), 1) < rest.indexOf(QLatin1Char('>'), 1) && rest.indexOf(QLatin1Char('<'), 1) != -1)) {
                    encodedSequence.append(c);
                    continue;
                }
                insideTag = true;
                continue;
            } else {
                encodedSequence.append(c);
            }
        }
    }

    return encodedSequence;
}

const QString KeyParser::decodeKeySequence(const QString &keys) const
{
    QString ret;

    for (int i = 0; i < keys.length(); i++) {
        QChar c = keys.at(i);
        int keycode = c.unicode();

        if ((keycode & 0xE000) != 0xE000) {
            ret.append(c);
        } else {
            ret.append(QLatin1Char('<'));

            if ((keycode & 0x1) == 0x1) {
                ret.append(QLatin1String("s-"));
                // keycode -= 0x1;
            }
            if ((keycode & 0x2) == 0x2) {
                ret.append(QLatin1String("c-"));
                // keycode -= 0x2;
            }
            if ((keycode & 0x4) == 0x4) {
                ret.append(QLatin1String("a-"));
                // keycode -= 0x4;
            }
            if ((keycode & 0x8) == 0x8) {
                ret.append(QLatin1String("m-"));
                // keycode -= 0x8;
            }

            if ((keycode & 0xE000) == 0xE000) {
                ret.append(m_keyCodeToName.value((keycode - 0xE000) / 0x10));
            } else {
                ret.append(QChar(keycode));
            }
            ret.append(QLatin1Char('>'));
        }
    }

    return ret;
}

const QChar KeyParser::KeyEventToQChar(const QKeyEvent &keyEvent)
{
    const int keyCode = keyEvent.key();
    const QString &text = keyEvent.text();
    const Qt::KeyboardModifiers mods = keyEvent.modifiers();

    // If previous key press was AltGr, return key value right away and don't go
    // down the "handle modifiers" code path. AltGr is really confusing...
    if (mods & Qt::GroupSwitchModifier) {
        return (!text.isEmpty()) ? text.at(0) : QChar();
    }

    if (text.isEmpty() || (text.length() == 1 && text.at(0) < 0x20) || keyCode == Qt::Key_Delete
        || (mods != Qt::NoModifier && mods != Qt::ShiftModifier && mods != Qt::KeypadModifier)) {
        QString keyPress;

        keyPress.append(QLatin1Char('<'));
        keyPress.append((mods & Qt::ShiftModifier) ? QStringLiteral("s-") : QString());
        keyPress.append((mods & Qt::ControlModifier) ? QStringLiteral("c-") : QString());
        keyPress.append((mods & Qt::AltModifier) ? QStringLiteral("a-") : QString());
        keyPress.append((mods & Qt::MetaModifier) ? QStringLiteral("m-") : QString());
        keyPress.append(keyCode <= 0xFF ? QChar(keyCode) : qt2vi(keyCode));
        keyPress.append(QLatin1Char('>'));

        return encodeKeySequence(keyPress).at(0);
    } else {
        return text.at(0);
    }
}
