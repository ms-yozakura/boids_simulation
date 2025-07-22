cmake_minimum_required(VERSION 3.10)
project(FlockingCreatures CXX C) # ここに 'C' を追加します

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 実行ファイルを作成
add_executable(FlockingCreatures)

# ソースファイルをターゲットに追加
target_sources(FlockingCreatures PRIVATE
    src/main.cpp
    src/Creature.cpp
    src/Shader.cpp
    third_party/glad/src/glad.c # Glad のソースファイルを明示的に追加
)

# glad.c を明示的に C 言語としてコンパイルするように指定
set_source_files_properties(third_party/glad/src/glad.c PROPERTIES LANGUAGE C)

# Glad のインクルードディレクトリを追加
target_include_directories(FlockingCreatures PUBLIC
    third_party/glad/include # Glad ヘッダーのパス
)

# GLFWライブラリをリンク
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(FlockingCreatures PRIVATE glfw)

# GLM のインクルードディレクトリを追加
target_include_directories(FlockingCreatures PRIVATE "/opt/homebrew/include")

# OpenGL フレームワークをリンク (macOS 特有)
find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
target_link_libraries(FlockingCreatures PRIVATE ${OPENGL_FRAMEWORK})

# --- OpenMP の設定 (Apple Silicon + Homebrew Clang 用)
set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include")
set(OpenMP_C_LIB_NAMES "omp")
set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include")
set(OpenMP_CXX_LIB_NAMES "omp")
set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
set(OpenMP_omp_INCLUDE_DIRS "/opt/homebrew/opt/libomp/include")

find_package(OpenMP REQUIRED)

# OpenMP ライブラリをターゲットにリンク
if(OpenMP_CXX_FOUND)
    target_link_libraries(FlockingCreatures PRIVATE OpenMP::OpenMP_CXX)
endif()

# ビルドディレクトリの設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# シェーダーファイルをビルドディレクトリにコピーする
# 実行ファイルからの相対パスに配置されるようにする
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/")

# (オプション) デバッグ用の設定 - 問題解決後には削除してOK
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v") # コンパイラ詳細表示
# set(CMAKE_VERBOSE_MAKEFILE ON) # ビルドコマンドの詳細表示