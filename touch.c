/*
 *  ########## touchコマンドのオプションは以下: ###########
 *  ### touch.exeはWindows専用のコマンドです。ファイルを作成したりします。 ###
 *
 *  -h  touchの説明とオプションを表示
 *
 *  -d　日時を指定する
 *
 *  -c ファイルを新規作成しない
 *
 *  -r 他のファイルと同じタイムスタンプにする
 *
 *  Author: physics11688 - 15.12.2022
 */

#define NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <windef.h>   // HWNDの定義.
#include <windows.h>  // windows用
#include <assert.h>

#define SIZE_OF_ARRAY(array) (sizeof(array) / sizeof(array[0]))
#define rm_zero_to_word(date) \
    (*date == '0' && *(date + 1) != '\0') ? (WORD)strtol(date + 1, NULL, 10) : (WORD)strtol(date, NULL, 10)

// 引数
#define help 'h'
#define no_create 'c'
#define specify_date 'd'
#define reference 'r'


// copy_referenceのステータス
enum copy_reference_Status {
    succes,
    get_handle_err,
    get_ref_filetime_err,
    set_filetime_err,
};

// datetime_to_FILEMTIMEとのステータス
enum datetime_to_FILEMTIME_Status {
    malloc_err = 1,
    get_timezone_info_err,
    conv_file_to_sys_time_err,

};

// datetime_to_FILEMTIMEとのステータス
enum do_touch_Status {
    get_systemtime_err = 1,
    get_filetime_err,
    conv_sys_to_file_time_err,

};

bool date_equal(SYSTEMTIME *lpstTime, SYSTEMTIME *stFileTime);
HANDLE get_handle(char *path, bool opt_no_create, bool is_folder);
uint8_t copy_reference(HANDLE hFile, char *ref_path);
bool copy_spc_date(HANDLE hFile, FILETIME lpFileTime);
uint8_t do_touch(HANDLE hFile);
uint8_t datetime_to_FILEMTIME(char *spc_date, FILETIME *lpFileTime);


int main(int argc, char *argv[]) {
    char *usage =
        "\n---- ファイルのタイムスタンプを変更するコマンド ----\n\n基本的な使い方: \
touch.exe [ファイル]\n\nオプション:\n\n   \
-h  touchの説明とオプションを表示\n\n   \
-c  ファイルを新規作成しない\n\n   -d　タイムスタンプの指定  \n        ex: touch -d \"2021-11-9 9:21\" file\n\n   \
-r  他のファイルのスタンプに合わせる\n        ex: touch -r ref_file file\n\n";
    int8_t opt;
    opterr = 0;  // 　getopt()のエラーメッセージを無効にする。

    // オプションのフラグ
    bool opt_no_create, opt_specify_date, opt_reference;
    opt_no_create = opt_specify_date = opt_reference = false;

    // optargを拾う用の変数
    char *spc_date;
    char *ref_path;


    // オプションの解析
    while ((opt = getopt(argc, argv, "hd:cr:")) != -1) {
        switch (opt) {
            case help:  // helpの表示
                puts(usage);
                return EXIT_SUCCESS;
            case no_create:
                opt_no_create = true;
                break;
            case specify_date:
                opt_specify_date = true;
                spc_date         = optarg;
                break;
            case reference:
                opt_reference = true;
                ref_path      = optarg;
                break;
            default:
                puts("存在しないオプションが指定されました");
                puts("run: touch -h");
                return EXIT_FAILURE;
        }
    }

    // pathが渡されてない場合はメッセージを表示してreturn
    if (argv[optind] == NULL) {
        puts("touch: ファイルオペランドがありません");
        return EXIT_FAILURE;
    }

    // coreutilesのtouchは、-d,-cが同時に渡されると後の方を優先
    if (opt_specify_date && opt_reference) {
        puts("オプション -d と -r を同時に指定することは出来ません");
        return EXIT_FAILURE;
    }


    // オプション以外の引数の処理
    uint8_t i;
    uint8_t subroutine_result;
    for (i = optind; i < argc; i++) {
        uint8_t result = EXIT_FAILURE;
        // ファイルハンドルの取得
        HANDLE hFile = get_handle(argv[i], opt_no_create, false);
        // 本当はファイルの存在確認まじでやったほうが良い
        if ((hFile == INVALID_HANDLE_VALUE) && (opt_no_create)) {
            // ファイルオープンに失敗 -> 存在しないファイル
            return EXIT_SUCCESS;
        } else if (hFile == INVALID_HANDLE_VALUE) {
            // たんなるファイルオープンの失敗
            return EXIT_FAILURE;
        }


        if (opt_reference == true) {  // rオプションが指定されたら
            switch (subroutine_result = copy_reference(hFile, ref_path)) {
                case get_handle_err:
                    puts("参照先のファイルが存在しないかもしれません");
                    break;
                case get_ref_filetime_err:
                    puts("参照先ファイルのタイムスタンプが取得できません");
                    break;
                case set_filetime_err:
                    puts("タイムスタンプを更新できません");
                    break;
                case succes:
                    result = EXIT_SUCCESS;
                    break;
            }
        } else if (opt_specify_date == true) {  // dオプションが指定されたら
            FILETIME lpFileTime = {0};
            switch (subroutine_result = datetime_to_FILEMTIME(spc_date, &lpFileTime)) {
                case malloc_err:
                    puts("メモリの確保に失敗しました");
                    break;
                case get_timezone_info_err:
                    puts("タイムゾーン情報の取得に失敗しました");
                    break;
                case conv_file_to_sys_time_err:
                    puts("指定の日付の変換に失敗しました");
                    break;
                case succes:
                    if (copy_spc_date(hFile, lpFileTime) == true) {
                        result = EXIT_SUCCESS;
                    }
                    break;
            }
        } else {  // オプション無しの処理
            switch (subroutine_result = do_touch(hFile)) {
                case get_systemtime_err:
                    puts("システム時刻の取得に失敗しました");
                    break;
                case get_filetime_err:
                    puts("ファイル時刻の取得に失敗しました");
                    break;
                case conv_sys_to_file_time_err:
                    puts("時刻の変換に失敗しました");
                    break;
                case succes:
                    result = EXIT_SUCCESS;
                    break;
            }
        }

        // それぞれの関数の処理でreturnすると明示的にclose出来ないのでここでする
        CloseHandle(hFile);
        // 引数のファイルの処理が失敗したら後続の引数を無視してreturn
        if (result == EXIT_FAILURE) {
            return result;
        }
    }

    // すべての引数に対して処理が成功したら成功コードを返す
    return EXIT_SUCCESS;
}

/*
 * CreateFileAのラッパー. 失敗するとINVALID_HANDLE_VALUEが帰る
 */
HANDLE get_handle(char *path, bool opt_no_create, bool is_folder) {
    assert(path != NULL);
    /*
    PCTSTR pszFileName    ファイル名。UNICODE が定義されてるはずなので PCWSTR。
    DWORD dwAccess        アクセス指定
    DWORD dwShare 共有方法.後続のオープン操作で読み取りアクセスが要求されたら?
    PSECURITY_ATTRIBUTES  セキュリティ属性
    DWORD dwCreatDisposition    動作指定
    dwCreatDisposition          ファイルをオープンすか？
    DWORD dwFlagsAndAttributes  フラグと属性
    HANDLE hTemplate            テンプレートファイル
    */

    PCTSTR pszFileName = path;                          // UNICODE が定義されてるはずなので PCWSTR。
    DWORD dwAccess     = GENERIC_READ | GENERIC_WRITE;  // データの読み書きとファイルポインタの移動.
    DWORD dwShare      = FILE_SHARE_READ | FILE_SHARE_WRITE;  // オープンを許可.
    PSECURITY_ATTRIBUTES psa = NULL;                          // デフォルトのセキュリティ記述子.
    DWORD dwCreatDisposition;                                 // 動作指定
    if (opt_no_create) {
        dwCreatDisposition = OPEN_EXISTING;  // no_createが付いてるときは存在してたら開く
    } else {
        dwCreatDisposition = OPEN_ALWAYS;  // ファイルオープン。ファイルが存在していない場合は新規作成する
    }

    DWORD dwFlagsAndAttributes;
    if (is_folder == true) {
        // ディレクトリのハンドル取得
        dwFlagsAndAttributes = FILE_FLAG_BACKUP_SEMANTICS;
    } else {
        dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    HANDLE hTemplate = NULL;

    // CloseHandleは copy_reference, copy_spc_date, do_touchで呼ぶ
    return CreateFileA(pszFileName, dwAccess, dwShare, psa, dwCreatDisposition, dwFlagsAndAttributes, hTemplate);
}

/*
 * rオプション専用の関数
 * ref_path のファイルタイムを取得して hFile のファイルに書き写す
 */
uint8_t copy_reference(HANDLE hFile, char *ref_path) {
    assert(hFile != INVALID_HANDLE_VALUE);
    assert(ref_path != NULL);

    // ref_path のファイルハンドルを取得
    // CreatFileA
    HANDLE ref_hFile = {0};
    bool is_folder;
    if (GetFileAttributes(ref_path) == FILE_ATTRIBUTE_DIRECTORY) {
        is_folder = true;
    } else {
        is_folder = false;
    }
    ref_hFile = get_handle(ref_path, true, is_folder);


    // ハンドル取得失敗
    if (ref_hFile == INVALID_HANDLE_VALUE) {
        return get_handle_err;
    }

    // ref_path のタイムスタンプ(LastWriteTime)を取得
    FILETIME ftFileTime;
    bool can_get_ref_time = GetFileTime(ref_hFile, NULL, NULL, &ftFileTime);
    CloseHandle(ref_hFile);
    if (can_get_ref_time == false) {
        return get_ref_filetime_err;
    }

    // hFile にスタンプ書き込み
    if (SetFileTime(hFile, NULL, NULL, &ftFileTime) == false) {
        return set_filetime_err;
    }


    return succes;
}

/*
 * dオプション専用の関数
 * 文字列 2012-9-12 14:02 とかをSYSTEMTIMEに変換して書き込む
 * {"2012", "9", "12", "14", "02"}
 */
uint8_t datetime_to_FILEMTIME(char *spc_date, FILETIME *lpFileTime) {
    assert(spc_date != NULL);
    // strlenの返すバイト長は'\0'を含まないもの
    char *cp_spc_date = (char *)malloc(strlen(spc_date) + sizeof('\0'));

    if (cp_spc_date == NULL) {
        perror("malloc:");
        return malloc_err;
    }

    // pathのコピー . strcpyは \0 を末尾に追加する。
    strcpy(cp_spc_date, spc_date);

    char *result[5] = {NULL};
    // size_t result_size = SIZE_OF_ARRAY(result);
    char *separator[] = {"-", " ", ":"};


    assert(separator != NULL);
    assert(result != NULL);
    assert(result_size > 0);

    size_t s_len = strlen(cp_spc_date);
    size_t start = 0;
    size_t end   = 0;
    size_t i     = 0;

    do {
        // 区切り文字でない文字が何文字続いているか調べて
        // 変数 end に加算。加算後の位置に区切り文字
        size_t tmp;
        size_t pos;
        uint8_t l = 0;
        pos       = strcspn(&cp_spc_date[start], *separator);
        // 区切り文字が複数
        for (l = 1; l <= 2; l++) {
            tmp = strcspn(&cp_spc_date[start], *(separator + l));
            if (tmp < pos) {
                pos = tmp;
            }
        }
        end = start + pos;

        // 区切り文字をヌル文字で上書き
        cp_spc_date[end] = '\0';

        // 分割後の文字列の先頭アドレスを result へ格納
        assert(i < result_size);
        result[i] = &cp_spc_date[start];
        ++i;

        // 次に調べる位置を設定
        start = end + 1;

    } while (start <= s_len);

    // {"2012", "9", "12", "14", "02"}
    SYSTEMTIME SysTime;

    SysTime.wYear         = (WORD)strtol(result[0], NULL, 10);  // 2012
    SysTime.wMonth        = rm_zero_to_word(result[1]);         // 9
    SysTime.wDay          = rm_zero_to_word(result[2]);         // 12
    SysTime.wHour         = rm_zero_to_word(result[3]);         // 14
    SysTime.wMinute       = rm_zero_to_word(result[4]);         // 2
    SysTime.wSecond       = 0;
    SysTime.wMilliseconds = 0;


    // timezone infoの取得
    TIME_ZONE_INFORMATION TimeZoneInfo;
    if (GetTimeZoneInformation(&TimeZoneInfo) == TIME_ZONE_ID_INVALID) {
        return get_timezone_info_err;
    }

    // ローカルタイムをUTCに変換
    SYSTEMTIME GmtTime = {0};
    if (TzSpecificLocalTimeToSystemTime(&TimeZoneInfo, &SysTime, &GmtTime) == 0) {
        printf("touch: `%s': 無効な日付の書式です\n", spc_date);
        free(cp_spc_date);
        exit(1);
    }
    free(cp_spc_date);


    if (SystemTimeToFileTime(&GmtTime, lpFileTime) == false) {
        return conv_file_to_sys_time_err;
    }

    return succes;
}

/*
 * dオプション専用の関数
 * datetime_to_FILEMTIME() で取得したFILETIMEを hFILEに書き込む
 */
bool copy_spc_date(HANDLE hFile, FILETIME lpFileTime) {
    if (SetFileTime(hFile, NULL, &lpFileTime, &lpFileTime) == false) {
        return false;
    }
    CloseHandle(hFile);
    return true;
}


// 構造体の比較
bool date_equal(SYSTEMTIME *lpstTime, SYSTEMTIME *stFileTime) {
    return lpstTime->wYear == stFileTime->wYear && lpstTime->wMonth == stFileTime->wMonth &&
           lpstTime->wDayOfWeek == stFileTime->wDayOfWeek && lpstTime->wDay == stFileTime->wDay &&
           lpstTime->wHour == stFileTime->wHour && lpstTime->wMinute == stFileTime->wMinute;
}


/*
 * 現在時刻でファイル作成
 *
 */
uint8_t do_touch(HANDLE hFile) {
    // 現在時刻を取得。処理はすべてUTC。
    SYSTEMTIME lpstTime;
    FILETIME ftTime;
    GetSystemTime(&lpstTime);
    if (SystemTimeToFileTime(&lpstTime, &ftTime) == false) {
        return get_systemtime_err;
    }


    // ファイルのタイムスタンプ(LastWriteTime)を取得
    FILETIME ftFileTime;
    SYSTEMTIME stFileTime;
    if (GetFileTime(hFile, NULL, NULL, &ftFileTime) == false) {
        return get_filetime_err;
    }

    if (FileTimeToSystemTime(&ftFileTime, &stFileTime) == false) {
        return conv_sys_to_file_time_err;
    }

    // 分まで現在時刻と等しいなら
    if (date_equal(&lpstTime, &stFileTime) == true) {
        // 何もしない
        return succes;
    } else {
        // タイムスタンプの更新
        SetFileTime(hFile, NULL, &ftTime, &ftTime);
    }

    return succes;
}
