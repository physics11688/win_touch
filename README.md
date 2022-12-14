# win_touch

Windows用のtouchです.

WSLがあると言っても,まあ一応ってことです.

でも本当は `<windows.h>` を使ってみるために昔遊びで書いただけです.



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