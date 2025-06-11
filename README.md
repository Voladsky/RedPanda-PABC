# RedPanda PABC.NET

Кроссплатформенная среда для языка PascalABC.NET на основе RedPanda C++.

# Установка

## Windows

1. Скачать .zip архив из последнего [https://github.com/Voladsky/RedPanda-PABC/releases/tag/v0.1.0](релиза), распаковать его в удобную папку
2. Запустить файл ReleaseQT\RedPanda\RedPandaIDE.exe

## Linux
0. Установить `mono` в `/usr/bin/mono` (проверить, куда установлено `mono` можно командой `which mono`) 
1. Скачать .AppImage файл из последнего [https://github.com/Voladsky/RedPanda-PABC/releases/tag/v0.1.0](релиза)
2. Открыть терминал в папке с файлом
3. Назначить файл исполняемым:
   ```bash
   chmod +x ./release_linux_portable.AppImage 
   ```
4. Запустить файл:
   ```bash
   ./release_linux_portable.AppImage 
   ```

Если получаете ошибку отсутствия FUSE, доустановите `libfuse2` пакетным менеджером своего дистрибутива. Для Ubuntu/Debian это:
```bash
sudo apt install libfuse2
```

Если не получается установить FUSE, AppImage можно распаковать и запустить бинарный файл напрямую:
```bash
./release_linux_portable.AppImage --appimage-extract
cd squashfs-root/usr/bin
./RedPandaIDE
```

--
