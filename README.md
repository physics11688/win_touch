# win_touch

Windows用のtouchです.

WSLがあると言っても,まあ一応ってことです.

でも本当は `<windows.h>` を使ってみるために昔遊びで書いただけです.



<br>

## Requirements

1. [Mingw-w64](https://www.mingw-w64.org/)をインストール済み
2. gccにパスが通っている

<br>

## Install

```bash
> git clone git@github.com:physics11688/win_touch.git

# ↑ が出来ない人は
> git clone https://github.com/physics11688/win_touch.git

> cd win_touch

# ビルド
> gcc -std=c11 -Wall -Wextra -fexec-charset=cp932 touch.c -o $HOME\local\bin\touch.exe

# 設置場所
> mkdir -p $HOME\local\bin

# バイナリの移動
> Move-Item -force .\touch.exe "$HOME\local\bin"

# profileの確認
> Test-Path $profile # Trueなら次へ進む. Falseなら→ New-Item -path $profile -type file -force

# エイリアスの設定
> echo 'Set-Alias touch "$HOME\local\bin\touch.exe"' >> $profile

# profileの読み込み
> . $profile

```

<br>

## Usage


```bash
> touch -h

---- ファイルのタイムスタンプを変更するコマンド ----

基本的な使い方: touch.exe [ファイル]

オプション:

   -h  touchの説明とオプションを表示

   -c  ファイルを新規作成しない

   -d　タイムスタンプの指定
        ex: touch -d "2021-11-9 9:21" file

   -r  他のファイルのスタンプに合わせる
        ex: touch -r ref_file file

```