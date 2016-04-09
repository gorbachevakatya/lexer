#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

using namespace std;

fstream in("input.txt");
//ofstream out("output.txt");
int CurrentLine = 1;
int CurrentColumn = 0;
char symbol = '~';  // Текущий символ

string lexem = "";  // Текущая лексема
string type = "";   // Тип текущей лексемы
int Line, Column;   // Первый символ текущей лексемы

string gnbCond;     // Состояние автомата процедуры gnb (GetNonBlank)
string ErrorMessage;

char nextsym(){     // Взять очередной символ из входного потока
    in>>symbol;
    CurrentColumn++;
    if (symbol == '\n') {
        CurrentColumn = 0; CurrentLine++;}
    if (in.eof()) symbol = '~';         // Символ конца файла
    return symbol;
}

void gnb () {                            // Пропустить пробелы и комментарии
    gnbCond = "begin";
    while (gnbCond != "end") {
        if (gnbCond == "begin") {
            switch (symbol) {
                case ' ':
                case '\n':
                case '\t':
                case '\r':                      nextsym();  break;
                case '{':   gnbCond = "C1";     nextsym();  break;
                case '\\':  gnbCond = "C2B";    nextsym();  break;
                case '/':   gnbCond = "C3B";                break;
                default:    gnbCond = "end";
            }
        }
        else if (gnbCond == "C1") {     // { ... }
            switch (symbol) {
                case '}':   gnbCond = "begin";  nextsym();  break;
                case '~':   gnbCond = "Error"; ErrorMessage = "Unsufficient EOF";  break;
                default:    nextsym();
            }
        }
        else if (gnbCond == "C2B") {  // /*...*/
            if (symbol == '*') {
                nextsym();
                gnbCond = "C2";
            }
            else {
                gnbCond = "Error";
                ErrorMessage = "Must be *";
            }
        }
        else if (gnbCond == "C2") {
            switch (symbol) {
                case '*':   gnbCond = "C2E";  nextsym();  break;
                case '~':   gnbCond = "Error"; ErrorMessage = "Unsufficient EOF";  break;
                default:    nextsym();
            }
        }
        else if (gnbCond == "C2E") {    // /* ... */
            switch (symbol) {
                case '\\':  gnbCond = "begin";  nextsym();  break;
                case '~':   gnbCond = "Error"; ErrorMessage = "Unsufficient EOF";  break;
                default:    gnbCond = "C2";
            }
        }
        else if (gnbCond == "C3B") {    // // ...
            if (in.peek() == '/')
                gnbCond = "C3";
            else
                gnbCond = "end";
        }
        else if (gnbCond == "C3") {     // // ...
            if (symbol == '\n' || symbol == '~')
                gnbCond = "begin";
            else
                nextsym();
        }
        else if (gnbCond == "Error") {
            cout<<CurrentLine<<'\t'<<CurrentColumn<<'\t'<<ErrorMessage<<'\n'<<endl;
            exit(1);
        }
    }
}

void ident () {     // Разбор идентификатора
    lexem = "";
    while (isalpha(symbol) || isdigit(symbol) || symbol == '_') {
        lexem += symbol;
        nextsym();
    }
    type = "ident";
}

void decint () {    // Разбор целого числа без знака
    lexem = "";
    while (isdigit(symbol)) {
        lexem += symbol;
        nextsym();
    }
    type = "integer";
}

void number () {
    string llex;
    decint();
    if (symbol == '.' || symbol == 'e' || symbol == 'E') {    // вещественное
        llex = lexem;
        if (symbol == '.') {
            nextsym();      // skip dot
            decint();
            llex = llex + "." + lexem;
        }
        if (symbol == 'e' || symbol == 'E') {
            llex += symbol;
            nextsym();      // skip E
            if (symbol == '+' || symbol == '-') {
                llex += symbol;
                nextsym();  // skip sign
            }
            decint();
            llex += lexem;
        }
        lexem = llex;
        type = "real";
    }
}

void hexint () {    // Разбор шестнадцатиричного числа
    lexem = "$";
    nextsym();
    while (isxdigit(symbol)) {
        lexem += symbol;
        nextsym();
    }
    type = "hex";
}

void litera_or_string () {    // Разбор 'символ' или '{символ}*'
    lexem = "\"";
    nextsym();
    while (symbol != '\'') {
        lexem += symbol;
        nextsym();
    }
    nextsym();
    lexem += "\"";
    if (lexem.length() != 3)
        type = "string";
    else
        type = "char";
}

void litera () {    // Разбор # [{число}{шестнадцатиричное_число}]
    nextsym();
    if (isdigit(symbol)) {
        decint();
    }
    else if (symbol == '$') {
        hexint();
    }
    else    lexem = "impossible";
    lexem = "#" + lexem;
    type = "char";
}

void SingleSymbol (string ttt) {lexem = symbol; type = ttt; nextsym();};

void ParsPlusMinusMulDiv (){
    SingleSymbol("op");
    if (symbol == '=') { lexem += symbol; nextsym();}
}

void ParsLess (){
    SingleSymbol("op");
    if (symbol == '=' || symbol == '>') { lexem += symbol; nextsym();}
}

void ParsDot (){
    SingleSymbol("op");
    if (symbol == '.') { lexem += symbol; nextsym();    type = "sep";}
}

void GetToken(){    // Разбор лексемы
    gnb ();
    Line = CurrentLine; Column = CurrentColumn;
    if (isalpha(symbol) || symbol == '_')   ident ();
    else if (isdigit(symbol)) number ();
    else if (symbol == '$') hexint ();
    else if (symbol == '\'') litera_or_string ();
    else if (symbol == '#') litera();
    else if (symbol == '(' || symbol == ')' || symbol == '[' || symbol == ']' || symbol == ';' || symbol == ',') SingleSymbol("sep");
    else if (symbol == '@' || symbol == '^' || symbol == '=') SingleSymbol("op");
    else if (symbol == '+' || symbol == '-' || symbol == '*' || symbol == '/' || symbol == '>' || symbol == ':') ParsPlusMinusMulDiv ();
    else if (symbol == '<') ParsLess ();
    else if (symbol == '.') ParsDot  ();
    else if (symbol == '~') {type = "EOF"; lexem = symbol;}
    else {   SingleSymbol("garbage");
    }
    if (lexem == ":") type = "sep";
}

int main(){
    in >> noskipws;
    nextsym ();
    while(symbol != '~') {
        GetToken();
        cout<<Line<<'\t'<<Column<<'\t'<<lexem<<'\t'<<type<<endl;
        lexem ="";
    }
}