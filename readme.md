
## $概要

三次元空間におけるBoidモデルをシミュレーションし、その描画をおこなうプロジェクトです。<br>
学校のプログラミング実践演習での課題にて、openGLとopenMPをつかった統合プロジェクトとして作成しました。

## $動作環境
MacBook Air 第２世代（Apple Silicon M2: macOS15.5）にて動作確認済み。

## $使用ライブラリ

- OpenGL    : グラフィックスAPI\
- GLM       : ベクトル/行列計算\
- GLFW      : ウィンドウ/入力管理\
- GLAD      : OpenGL関数ローダー　　（→ ./flocking_vx/third_party/glad/ 内にある）\

- OpemMP    : マルチスレッド並列処理 

<br><br>

## $ビルド方法・実行方法

ターミナル上で

>% cd flocking_vx %xは任意のバージョン\
% mkdir build %buildが存在しないときは行ってください\
% cd build\
% cmake .. \
-DCMAKE_C_COMPILER=clang \\\
-DCMAKE_CXX_COMPILER=clang++ \\\
-DCMAKE_C_FLAGS="-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include" \\\
-DCMAKE_CXX_FLAGS="-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include" \\\
-DCMAKE_EXE_LINKER_FLAGS="-L/opt/homebrew/opt/libomp/lib -lomp"\
% make\
% ./FlockingCreatures
<br><br>

**<<補足>>**

buildに入って

>% cmake .. % v3以前はこれでok。v4からはこれでは通らない\

したいところだけど、appleでは標準でopenmpが使えないので、コマンドからプリプロセッサとして-fopenmpを渡す必要がある。\
そのため、下のコマンドを使う。\
<br>
オプション部分は上から順に\
    1,2行目：　clangコンパイラの指定（C/C++）\
    3,4行目：　Xpreprocessorに-fopenmpを渡し、インクルードパスにlibompのヘッダファイルディレクトリを指定（C/C++）\
    5行目：　リンク時の設定。リンクするライブラリの位置とファイルを指定\
である

>% cmake .. \
-DCMAKE_C_COMPILER=clang \
-DCMAKE_CXX_COMPILER=clang++ \
-DCMAKE_C_FLAGS="-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include" \
-DCMAKE_CXX_FLAGS="-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include" \
-DCMAKE_EXE_LINKER_FLAGS="-L/opt/homebrew/opt/libomp/lib -lomp"
