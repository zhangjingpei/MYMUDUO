#!/bin/bash

# `#!/bin/bash` - 指定脚本使用Bash解释器执行
# `set -e` - 设置脚本在发生错误时立即退出（若任何命令以非零状态退出，则脚本终止）

# 如果没有build目录 创建该目录
# `[ ! -d "$(pwd)/build" ]` - 检查当前目录下是否存在build目录
#   `!` 表示逻辑非
#   `-d` 检查路径是否存在且为目录
#   `$(pwd)` 执行pwd命令获取当前工作目录
if [ ! -d "$(pwd)/build" ]; then
    # `mkdir` 创建目录
    mkdir "$(pwd)/build"
fi

# 清理build目录内容
# `rm -rf` - 递归强制删除
#   `-r` 递归删除
#   `-f` 强制删除不提示
# `/*` - 匹配目录下所有文件和子目录
rm -rf "$(pwd)/build"/*

# 进入build目录并编译项目
# `cd ... &&` - 若cd成功则执行后续命令
#   `&&` 逻辑与操作符（前一条命令成功才执行后一条）
# `cmake ..` - 在build目录中生成Makefile（..表示上级目录）
# `make` - 执行编译
cd "$(pwd)/build" && 
    cmake .. && 
    make

# 回到项目根目录
cd ..

# 清理并创建头文件安装目录
# `sudo` - 以超级用户权限执行命令
sudo rm -rf /usr/include/mymuduo  # 递归删除旧头文件目录
sudo mkdir -p /usr/include/mymuduo  # `-p` 创建多级目录（若不存在父目录则一并创建）

# 复制所有头文件（保留目录结构）
# `find .` - 在当前目录递归查找文件
#   `.` 表示当前工作目录
# `-name "*.h"` - 匹配所有.h后缀文件
# `-exec ... \;` - 对每个匹配文件执行指定命令（;需转义为\;）
# `cp --parents` - 复制时保留目录结构
#   `{}` - 被find找到的文件路径占位符
find . -name "*.h" -exec sudo cp --parents {} /usr/include/mymuduo \;

# 复制库文件到系统目录（根据用户说明库文件在lib目录下）
# `-name "*.so*"` - 匹配所有.so文件（包括带版本号的文件）
# `build/lib/` - 指定库文件所在目录
sudo find -name "*.so*" -exec sudo cp -v {} /usr/lib \;
#   `-v` 显示复制详情（可选）
# 注意：这里使用`build/lib/`而不仅是`build/`以精确定位库文件目录

# 更新动态库缓存
# `ldconfig` - 重建共享库缓存，使系统识别新安装的库
sudo ldconfig

echo "Installation completed successfully"  # 输出成功信息