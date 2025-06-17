/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "pas.h"
#include "../constants.h"
#include "qt_utils/utils.h"

#include <QFont>
#include <QDebug>

namespace QSynedit {

const QSet<QString> PasSyntaxer::Keywords {
    "and", "break", "do", "else", "elseif",
    "begin", "end", "false", "for", "function", "goto",
    "if", "in", "local", "nil", "not", "or",
    "repeat", "return", "then", "true", "until",
    "while", "new", "var", "to", "type"
};

const QSet<QString> PasSyntaxer::StdLibFunctions {
};

const QMap<QString,QSet<QString>> PasSyntaxer::StdLibTables {
};

const QSet<QString> PasSyntaxer::XMakeLibFunctions {
};


PasSyntaxer::PasSyntaxer(): Syntaxer()
{
    mUseXMakeLibs=false;
    mCharAttribute = std::make_shared<TokenAttribute>(SYNS_AttrCharacter,
                                                            TokenType::Character);
    addAttribute(mCharAttribute);

    mFloatAttribute = std::make_shared<TokenAttribute>(SYNS_AttrFloat,
                                                             TokenType::Number);
    addAttribute(mFloatAttribute);
    mHexAttribute = std::make_shared<TokenAttribute>(SYNS_AttrHexadecimal,
                                                           TokenType::Number);
    addAttribute(mHexAttribute);
    mInvalidAttribute = std::make_shared<TokenAttribute>(SYNS_AttrIllegalChar,
                                                               TokenType::Error);
    addAttribute(mInvalidAttribute);
    mNumberAttribute = std::make_shared<TokenAttribute>(SYNS_AttrNumber,
                                                              TokenType::Number);
    addAttribute(mNumberAttribute);

    mStringEscapeSequenceAttribute = std::make_shared<TokenAttribute>(SYNS_AttrStringEscapeSequences,
                                                                            TokenType::String);
    addAttribute(mStringEscapeSequenceAttribute);
    resetState();
}

const PTokenAttribute &PasSyntaxer::invalidAttribute() const
{
    return mInvalidAttribute;
}

const PTokenAttribute &PasSyntaxer::numberAttribute() const
{
    return mNumberAttribute;
}

const PTokenAttribute &PasSyntaxer::floatAttribute() const
{
    return mFloatAttribute;
}

const PTokenAttribute &PasSyntaxer::hexAttribute() const
{
    return mHexAttribute;
}

const PTokenAttribute &PasSyntaxer::stringEscapeSequenceAttribute() const
{
    return mStringEscapeSequenceAttribute;
}

const PTokenAttribute &PasSyntaxer::charAttribute() const
{
    return mCharAttribute;
}
PasSyntaxer::TokenId PasSyntaxer::getTokenId()
{
    return mTokenId;
}

void PasSyntaxer::commentProc()
{
    mTokenId = TokenId::Comment;
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    while (mRun<mLineSize) {
        if (isSpaceChar(mLine[mRun]))
            break;
        mRun++;
    }
    if (mRun<mLineSize) {
        mRange.state = RangeState::rsComment;
    } else
        mRange.state = RangeState::rsUnknown;
}

void PasSyntaxer::longCommentProc()
{
    mTokenId = TokenId::Comment;
    mRange.state=RangeState::rsLongComment;
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case ' ':
        case '\t':
            return;
        case ']':
            mRun ++;
            mRange.state = RangeState::rsUnknown;
            return;
        default:
            mRun+=1;
        }
    }
}

void PasSyntaxer::braceCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;

    mRange.braceLevel -= 1;
    if (mRange.braceLevel<0) {
        mRange.braceLevel = 0;
    }
}

void PasSyntaxer::braceOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;

    mRange.braceLevel += 1;
}

void PasSyntaxer::colonProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]==':') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void PasSyntaxer::equalProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '=') {
        mRun += 2;
    } else {
        mRun += 1;
    }
}

void PasSyntaxer::greaterProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch (mLine[mRun+1].unicode()) {
        case '=':
        case '>':
            mRun += 2;
            return;
        }
    }
    mRun+=1;
}

void PasSyntaxer::identProc()
{
    int wordEnd = mRun;
    while (wordEnd<mLineSize && isIdentChar(mLine[wordEnd])) {
        wordEnd+=1;
    }
    QString word = mLine.mid(mRun,wordEnd-mRun);
    mRun=wordEnd;
    if (isKeyword(word)) {
        mTokenId = TokenId::Key;
        if (word == "then" || word == "do" || word == "repeat" || word == "function") {
            mRange.blockLevel += 1;
            mRange.blockStarted++;
            pushIndents(IndentType::Block);
        } else if (word == "end" || word =="until") {
            mRange.blockLevel -= 1;
            if (mRange.blockLevel<0) {
                mRange.blockLevel = 0;
            }
            if (mRange.blockStarted>0) {
                mRange.blockStarted--;
            } else {
                mRange.blockEnded++ ;
            }
            popIndents(IndentType::Block);
        }
    } else {
        mTokenId = TokenId::Identifier;
    }
}

void PasSyntaxer::lowerProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize) {
        switch(mLine[mRun+1].unicode()) {
        case '=':
        case '<':
            mRun+=2;
            return;
        }
    }
    mRun+=1;
}

void PasSyntaxer::minusProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='-') {
        if (mRun+2<mLineSize && mLine[mRun+2]=='[') {
            mRun+=3;
            mTokenId = TokenId::Comment;
            mRange.state=RangeState::rsLongComment;
        } else {
            mRun+=2;
            mTokenId=TokenId::Comment;
            mRange.state = RangeState::rsComment;
        }
    } else
        mRun++;
}

void PasSyntaxer::nullProc()
{
    mTokenId = TokenId::Null;
}

void PasSyntaxer::numberProc()
{
    int idx1; // token[1]
    idx1 = mRun;
    mRun+=1;
    mTokenId = TokenId::Number;
    bool shouldExit = false;
    while (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '\'':
            if (mTokenId != TokenId::Number) {
                mTokenId = TokenId::Symbol;
                return;
            }
            break;
        case '.':
            if (mRun+1<mLineSize && mLine[mRun+1] == '.') {
                mRun+=2;
                mTokenId = TokenId::Unknown;
                return;
            } else if (mTokenId != TokenId::Hex) {
                mTokenId = TokenId::Float;
            } else {
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case '-':
        case '+':
            if (mTokenId != TokenId::Float) // number <> float. an arithmetic operator
                return;
            if (mRun-1>=0 && mLine[mRun-1]!= 'e' && mLine[mRun-1]!='E')  // number = float, but no exponent. an arithmetic operator
                return;
            if (mRun+1<mLineSize && (mLine[mRun+1]<'0' || mLine[mRun+1]>'9'))  {// invalid
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if ( (mLine[idx1]=='0') && (mTokenId != TokenId::Hex)  && (mTokenId != TokenId::Float) ) // invalid octal char
                mTokenId = TokenId::Unknown; // we must continue parse, it may be an float number
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
            if (mTokenId!=TokenId::Hex) { //invalid
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case 'e':
        case 'E':
            if (mTokenId!=TokenId::Hex) {
                if (mRun-1>=0 && (mLine[mRun-1]>='0' || mLine[mRun-1]<='9') ) {//exponent
                    for (int i=idx1;i<mRun;i++) {
                        if (mLine[i] == 'e' || mLine[i]=='E') { // too many exponents
                            mRun+=1;
                            mTokenId = TokenId::Unknown;
                            return;
                        }
                    }
                    if (mRun+1<mLineSize && mLine[mRun+1]!='+' && mLine[mRun+1]!='-' && !(mLine[mRun+1]>='0' && mLine[mRun+1]<='9')) {
                        return;
                    } else {
                        mTokenId = TokenId::Float;
                    }
                } else {
                    mRun+=1;
                    mTokenId = TokenId::Unknown;
                    return;
                }
            }
            break;
        case 'f':
        case 'F':
            if (mTokenId!=TokenId::Hex) {
                for (int i=idx1;i<mRun;i++) {
                    if (mLine[i] == 'f' || mLine[i]=='F') {
                        mRun+=1;
                        mTokenId = TokenId::Unknown;
                        return;
                    }
                }
                if (mTokenId == TokenId::Float) {
                    if (mRun-1>=0 && (mLine[mRun-1]=='l' || mLine[mRun-1]=='L')) {
                        mRun+=1;
                        mTokenId = TokenId::Unknown;
                        return;
                    }
                } else {
                    mTokenId = TokenId::Float;
                }
            }
            break;
        case 'l':
        case 'L':
            for (int i=idx1;i<=mRun-2;i++) {
                if (mLine[i] == 'l' && mLine[i]=='L') {
                    mRun+=1;
                    mTokenId = TokenId::Unknown;
                    return;
                }
            }
            if (mTokenId == TokenId::Float && (mLine[mRun-1]=='f' || mLine[mRun-1]=='F')) {
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        case 'u':
        case 'U':
            if (mTokenId == TokenId::Float) {
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            } else {
                for (int i=idx1;i<mRun;i++) {
                    if (mLine[i] == 'u' || mLine[i]=='U') {
                        mRun+=1;
                        mTokenId = TokenId::Unknown;
                        return;
                    }
                }
            }
            break;
        case 'x':
        case 'X':
            if ((mRun == idx1+1) && (mLine[idx1]=='0') &&
                    mRun+1<mLineSize &&
                    ((mLine[mRun+1]>='0' && mLine[mRun+1]<='9')
                     || (mLine[mRun+1]>='a' && mLine[mRun+1]<='f')
                     || (mLine[mRun+1]>='A' && mLine[mRun+1]<='F')) ) {
                mTokenId = TokenId::Hex;
            } else {
                mRun+=1;
                mTokenId = TokenId::Unknown;
                return;
            }
            break;
        default:
            shouldExit=true;
        }
        if (shouldExit) {
            break;
        }
        mRun+=1;        
    }
    if (mRun-1>=0 && mLine[mRun-1] == '\'') {
        mTokenId = TokenId::Unknown;
    }
}

void PasSyntaxer::pointProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1] == '.' ) {
        if (mRun+2<mLineSize && mLine[mRun+2] == '.')
                mRun+=3;
        else
            mRun+=2;
    } else {
        mRun+=1;
    }
}

void PasSyntaxer::roundCloseProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel--;
    if (mRange.parenthesisLevel<0)
        mRange.parenthesisLevel=0;
    popIndents(IndentType::Parenthesis);
}

void PasSyntaxer::roundOpenProc()
{
    mRun += 1;
    mTokenId = TokenId::Symbol;
    mRange.parenthesisLevel++;
    pushIndents(IndentType::Parenthesis);
}

void PasSyntaxer::slashProc()
{
    if (mRun+1<mLineSize && mLine[mRun+1]=='/')
        mRun += 2;
    else
        mRun++;
    mTokenId = TokenId::Symbol;
}

void PasSyntaxer::spaceProc()
{
    mRun += 1;
    mTokenId = TokenId::Space;
    while (mRun<mLineSize && isLexicalSpace(mLine[mRun]))
        mRun+=1;
    if (mRun>=mLineSize) {
        mRange.hasTrailingSpaces = true;
        if (mRange.state==RangeState::rsComment)
            mRange.state = RangeState::rsUnknown;
    }
}

void PasSyntaxer::squareCloseProc()
{
    mRun+=1;
    mTokenId = TokenId::Symbol;
    mRange.bracketLevel--;
    if (mRange.bracketLevel<0)
        mRange.bracketLevel=0;
    popIndents(IndentType::Bracket);
}

void PasSyntaxer::squareOpenProc()
{
    int i=mRun+1;
    while (i<mLineSize && mLine[i]=='=')
        i++;
    if (i<mLineSize && mLine[i]=='[') {
        mRange.state=RangeState::rsLongString+(i-mRun-1);
        mRun=i+1;
        mTokenId=TokenId::String;
    } else {
        mRun++;
        mTokenId = TokenId::Symbol;
        mRange.bracketLevel++;
        pushIndents(IndentType::Bracket);
    }
}

//void PasSyntaxer::stringEndProc()
//{
//    mTokenId = TokenId::String;
//    if (mRun>=mLineSize) {
//        nullProc();
//        return;
//    }
//    mRange.state = RangeState::rsUnknown;

//    while (mRun<mLineSize) {
//        if (mLine[mRun]=='\'') {
//            mRun += 1;
//            break;
//        } else if (isSpaceChar(mLine[mRun])) {
//            mRange.state = RangeState::rsString;
//            return;
//        } else if (mLine[mRun]=='\\') {
//            if (mRun == mLineSize-1) {
//                mRun+=1;
//                mRange.state = RangeState::rsMultiLineString;
//                return;
//            }
//            if (mRun+1<mLineSize) {
//                switch(mLine[mRun+1].unicode()) {
//                case '\'':
//                case '\'':
//                case '\\':
//                case '?':
//                case 'a':
//                case 'b':
//                case 'f':
//                case 'n':
//                case 'r':
//                case 't':
//                case 'v':
//                case '0':
//                case '1':
//                case '2':
//                case '3':
//                case '4':
//                case '5':
//                case '6':
//                case '7':
//                case '8':
//                case '9':
//                case 'x':
//                case 'u':
//                case 'U':
//                    mRange.state = RangeState::rsMultiLineStringEscapeSeq;
//                    return;
//                }
//            }
//        }
//        mRun += 1;
//    }
//}

void PasSyntaxer::stringEscapeSeqProc()
{
    mTokenId = TokenId::StringEscapeSeq;
    mRun+=1;
    if (mRun<mLineSize) {
        switch(mLine[mRun].unicode()) {
        case '\'':
        case '"':
        case '?':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\\':
            mRun+=1;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            for (int i=0;i<3;i++) {
                if (mRun>=mLineSize || mLine[mRun]<'0' || mLine[mRun]>'7')
                    break;
                mRun+=1;
            }
            break;
        case '8':
        case '9':
            mTokenId = TokenId::Unknown;
            mRun+=1;
            break;
        case 'x':
            mRun+=1;
            if (mRun>=mLineSize || !(
                     (mLine[mRun]>='0' && mLine[mRun]<='9')
                   ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                   ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                    )) {
                mTokenId = TokenId::Unknown;
            } else {
                while (mRun<mLineSize && (
                       (mLine[mRun]>='0' && mLine[mRun]<='9')
                     ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                     ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                       ))  {
                    mRun+=1;
                }
            }
            break;
        case 'u':
            mRun+=1;
            for (int i=0;i<4;i++) {
                if (mRun>=mLineSize || !(
                            (mLine[mRun]>='0' && mLine[mRun]<='9')
                          ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                          ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                           )) {
                    mTokenId = TokenId::Unknown;
                    return;
                }
                mRun+=1;
            }
            break;
        case 'U':
            mRun+=1;
            for (int i=0;i<8;i++) {
                if (mRun>=mLineSize || !(
                            (mLine[mRun]>='0' && mLine[mRun]<='9')
                          ||  (mLine[mRun]>='a' && mLine[mRun]<='f')
                          ||  (mLine[mRun]>='A' && mLine[mRun]<='F')
                           )) {
                    mTokenId = TokenId::Unknown;
                    return;
                }
                mRun+=1;
            }
            break;
        }
    }
    mRange.state = RangeState::rsString;
}

void PasSyntaxer::stringProc()
{
    if (mRun>=mLineSize) {
        nullProc();
        return;
    }
    mTokenId = TokenId::String;
    while (mRun < mLineSize) {
        if (mRange.state==RangeState::rsString && mLine[mRun]=='\'') {
            mRun++;
            break;
        } else if (mRange.state>=RangeState::rsLongString && mLine[mRun]==']') {
            int i=mRun+1;
            while (i<mLineSize && mLine[i]=='=')
                i++;
            if (i<mLineSize && mLine[i]==']' && (mRange.state-RangeState::rsLongString == i-mRun-1)) {
                mRun=i+1;
                mRange.state = RangeState::rsUnknown;
                return;
            }
        } else if (mLine[mRun]=='\\') {
            if (mRun+1<mLineSize) {
                switch(mLine[mRun+1].unicode()) {
                case '\'':
                case '"':
                case '\\':
                case '?':
                case 'a':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case 'x':
                case 'u':
                case 'U':
                    mRange.state = RangeState::rsStringEscapeSeq;
                    return;
                }
            }
        }
        mRun+=1;
    }
    if (mRange.state == RangeState::rsString)
        mRange.state = RangeState::rsUnknown;
}

void PasSyntaxer::tildeProc()
{
    mTokenId = TokenId::Symbol;
    if (mRun+1<mLineSize && mLine[mRun+1]=='=') {
        mRun+=2;
    } else {
        mRun+=1;
    }
}

void PasSyntaxer::unknownProc()
{
    mRun+=1;
    mTokenId = TokenId::Unknown;
}

void PasSyntaxer::processChar()
{
    if (mRun>=mLineSize) {
        nullProc();
    } else {
        switch(mLine[mRun].unicode()) {
        case '-':
            minusProc();
            break;
        case '+':
        case '*':
        case '%':
        case '^':
        case '#':
        case '&':
        case '|':
        case ',':
        case ';':
            mTokenId = TokenId::Symbol;
            mRun+=1;
            break;
        case '~':
            tildeProc();
            break;
        case '<':
            lowerProc();
            break;
        case '>':
            greaterProc();
            break;
        case '=':
            equalProc();
            break;
        case '/':
            slashProc();
            break;
        case ':':
            colonProc();
            break;
        case '.':
            pointProc();
            break;

        case '}':
            braceCloseProc();
            break;
        case '{':
            braceOpenProc();
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            numberProc();
            break;
        case ')':
            roundCloseProc();
            break;
        case '(':
            roundOpenProc();
            break;

        case ']':
            squareCloseProc();
            break;
        case '[':
            squareOpenProc();
            break;
        case '\'':
            mRange.state=RangeState::rsString;
            mTokenId=TokenId::String;
            mRun++;
            break;
        default:
            if (isIdentChar(mLine[mRun])) {
                identProc();
            } else {
                unknownProc();
            }
        }
    }
}

void PasSyntaxer::popIndents(IndentType indentType)
{
//    qDebug()<<"----";
//    for (IndentInfo info:mRange.indents)
//        qDebug()<<(int)info.type<<info.line;
//    qDebug()<<"****";
    while (!mRange.indents.isEmpty() && mRange.indents.back().type!=indentType) {
        mRange.indents.pop_back();
    }
    if (!mRange.indents.isEmpty()) {
        mRange.lastUnindent=mRange.indents.back();
        mRange.indents.pop_back();
    } else {
        mRange.lastUnindent=IndentInfo{indentType,0};
    }
}

void PasSyntaxer::pushIndents(IndentType indentType, int line)
{
    if (line==-1)
        line = mLineNumber;
    mRange.indents.push_back(IndentInfo{indentType,line});
}

bool PasSyntaxer::useXMakeLibs() const
{
    return mUseXMakeLibs;
}

void PasSyntaxer::setUseXMakeLibs(bool newUseXMakeLibs)
{
    if (mUseXMakeLibs!=newUseXMakeLibs) {
        mKeywordsCache.clear();
        mUseXMakeLibs = newUseXMakeLibs;
    }
}

QString PasSyntaxer::commentSymbol()
{
    return "--";
}

QString PasSyntaxer::blockCommentBeginSymbol()
{
    return "--[";
}

QString PasSyntaxer::blockCommentEndSymbol()
{
    return "]";
}

bool PasSyntaxer::supportFolding()
{
    return true;
}

bool PasSyntaxer::needsLineState()
{
    return false;
}

const QSet<QString> &PasSyntaxer::customTypeKeywords() const
{
    return mCustomTypeKeywords;
}

void PasSyntaxer::setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords)
{
    mCustomTypeKeywords = newCustomTypeKeywords;
    mKeywordsCache.clear();
}

bool PasSyntaxer::supportBraceLevel()
{
    return true;
}

QMap<QString, QSet<QString> > PasSyntaxer::scopedKeywords()
{
    return StdLibTables;
}

bool PasSyntaxer::isCommentNotFinished(int state) const
{
    return (state == RangeState::rsComment ||
            state == RangeState::rsLongComment);
}

bool PasSyntaxer::isStringNotFinished(int state) const
{
    return state == RangeState::rsString;
}

bool PasSyntaxer::eol() const
{
    return mTokenId == TokenId::Null;
}

QString PasSyntaxer::getToken() const
{
    return mLine.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &PasSyntaxer::getTokenAttribute() const
{
    switch (mTokenId) {
    case TokenId::Comment:
        return mCommentAttribute;
    case TokenId::Identifier:
        return mIdentifierAttribute;
    case TokenId::Key:
        return mKeywordAttribute;
    case TokenId::Number:
        return mNumberAttribute;
    case TokenId::Float:
    case TokenId::HexFloat:
        return mFloatAttribute;
    case TokenId::Hex:
        return mHexAttribute;
    case TokenId::Space:
        return mWhitespaceAttribute;
    case TokenId::String:
        return mStringAttribute;
    case TokenId::StringEscapeSeq:
        return mStringEscapeSequenceAttribute;
    case TokenId::Symbol:
        return mSymbolAttribute;
    case TokenId::Unknown:
        return mInvalidAttribute;
    default:
        return mInvalidAttribute;
    }
}

int PasSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void PasSyntaxer::next()
{
    mAsmStart = false;
    mTokenPos = mRun;
    do {
        if (mRun<mLineSize && isSpaceChar(mLine[mRun])) {
            spaceProc();
            break;
        }
        switch (mRange.state) {
        case RangeState::rsComment:
            //qDebug()<<"*0-0-0*";
            commentProc();
            break;
        case RangeState::rsString:
            //qDebug()<<"*1-0-0*";
            stringProc();
            break;
        case RangeState::rsLongComment:
            //qDebug()<<"*2-0-0*";
            longCommentProc();
            break;
//        case RangeState::rsMultiLineString:
//            //qDebug()<<"*4-0-0*";
//            stringEndProc();
//            break;
        case RangeState::rsStringEscapeSeq:
            //qDebug()<<"*6-0-0*";
            stringEscapeSeqProc();
            break;
        default:
            if (mRange.state>=RangeState::rsLongString) {
                stringProc();
            } else {
                //qDebug()<<"*a-0-0*";
                mRange.state = RangeState::rsUnknown;
                if (mRun>=mLineSize) {
                    //qDebug()<<"*b-0-0*";
                    nullProc();
                } else {
                    //qDebug()<<"*f-0-0*";
                    processChar();
                }
            }
        }
    } while (mTokenId!=TokenId::Null && mRun<=mTokenPos);
    //qDebug()<<"1-1-1";
}

void PasSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLine = newLine;
    mLineSize = mLine.size();
    mLineNumber = lineNumber;
    mRun = 0;
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
    next();
}

bool PasSyntaxer::isKeyword(const QString &word)
{
    return Keywords.contains(word) || mCustomTypeKeywords.contains(word);
}

void PasSyntaxer::setState(const SyntaxState& rangeState)
{
    mRange = rangeState;
    // current line's left / right parenthesis count should be reset before parsing each line
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
}

void PasSyntaxer::resetState()
{
    mRange.state = RangeState::rsUnknown;
    mRange.braceLevel = 0;
    mRange.bracketLevel = 0;
    mRange.parenthesisLevel = 0;
    mRange.blockLevel = 0;
    mRange.blockStarted = 0;
    mRange.blockEnded = 0;
    mRange.blockEndedLastLine = 0;
    mRange.indents.clear();
    mRange.lastUnindent=IndentInfo{IndentType::None,0};
    mRange.hasTrailingSpaces = false;
    mAsmStart = false;
}

QString PasSyntaxer::languageName()
{
    return "lua";
}

ProgrammingLanguage PasSyntaxer::language()
{
    return ProgrammingLanguage::LUA;
}

SyntaxState PasSyntaxer::getState() const
{
    return mRange;
}

bool PasSyntaxer::isIdentChar(const QChar &ch) const
{
    return ch=='_' || ch.isDigit() || ch.isLetter();
}

bool PasSyntaxer::isIdentStartChar(const QChar &ch) const
{
    return ch=='_' || ch.isLetter();
}

QSet<QString> PasSyntaxer::keywords() {
    if (mKeywordsCache.isEmpty()) {
        mKeywordsCache = Keywords;
        mKeywordsCache.unite(mCustomTypeKeywords);
        mKeywordsCache.unite(StdLibFunctions);
        if (mUseXMakeLibs)
            mKeywordsCache.unite(XMakeLibFunctions);
        foreach(const QString& s, StdLibTables.keys())
            mKeywordsCache.insert(s);
    }
    return mKeywordsCache;
}

QString PasSyntaxer::foldString(QString endLine)
{
    if (endLine.trimmed().startsWith("#"))
        return "...";
    return "...}";
}

}
