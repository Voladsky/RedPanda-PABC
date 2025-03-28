# RedPanda PABC.NET

Кроссплатформенная среда для языка PascalABC.NET на основе RedPanda C++

# Установка

Процесс установки:
1. Клонировать этот репозиторий
2. Клонировать репозиторий PascalABC.NET с поддержкой ZMQ [https://github.com/Voladsky/pascalabcnet-zmq.git](https://github.com/Voladsky/pascalabcnet-zmq.git)
3. Сгенерировать Linux-версию PascalABC.NET (скрипты GenerateLinuxVersion.bat и/или GenerateLinuxVersion.sh)
4. Собрать проект RedPanda, используя Qt Creator (можно и без Qt Creator, но, так или иначе, понадобится qmake)
5. Скопировать Linux-версию PascalABC.NET, сгенерированную на шаге 2, из Releases/PascalABCNETLinux в папку PascalABCNETLinux в папке с билдом RedPanda
6. Запустить RedPanda

**Возможная проблема с consolepauser**: если возникает ошибка, что consolepauser не найден в какой-либо папке, просто скопируйте исполняемый файл из папки с билдом /tools/consolepauser/{debug, release}/consolepauser{.exe} в папку, в которой RedPanda не может найти consolepauser (при необходимости нужно досоздать несуществующие папки).

Подробности по RedPanda C++ : [https://github.com/royqh1979/RedPanda-CPP](https://github.com/royqh1979/RedPanda-CPP)
Подробности по PascalABC.NET: [https://github.com/pascalabcnet/pascalabcnet](https://github.com/pascalabcnet/pascalabcnet)

--

Simplified Chinese Website: [http://royqh.net/redpandacpp](http://royqh.net/redpandacpp)

English Website: [https://sourceforge.net/projects/redpanda-cpp](https://sourceforge.net/projects/redpanda-cpp)

[Donate to this project](https://ko-fi.com/royqh1979)

New Features (Compared with Red Panda Dev-C++ 6):
* Cross Platform (Windows/Linux/MacOS)
* Problem Set (run and test program against predefined input / expected output data)
* Competitive Companion support ( It's an chrome/firefox extension that can fetch problems from OJ websites)
* Edit/compile/run/debug GNU Assembly language programs.
* Find symbol occurrences
* Memory View for debugging
* TODO View
* Support SDCC Compiler

UI Improvements:
* Full high-dpi support, including fonts and icons
* Better dark theme support
* Better editor color scheme support
* Redesigned Find/Replace in Files UI
* Redesigned bookmark UI

Editing Improvements:
* Enhanced auto indent 
* Enhanced code completion
* Better code folding support

Debugging Improvements:
* Use gdb/mi interface
* Enhanced watch
* gdbserver mode

Code Intellisense Improvements:
* Better support identifiers for complex expressions
* Support UTF-8 identifiers
* Support C++ 14 using type alias
* Support C-Style enum variable definitions
* Support MACRO with arguments
* Support C++ lambdas

And many other improvements and bug fixes. See NEWS.md for full information.
