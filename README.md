# GITLITE - A simplified version control system

本项目为 `SJTU CS1958-01 2025Fall` 的 `Project 4`，亦为 `UC Berkeley` 的 `CS61B` 课程的 `Gitlet` 项目，实现了对 `Git` 的简化模拟。

## 1. 项目结构

本项目的结构如下
```
gitlite/
├── build/                  # 编译输出目录 (make 生成的可执行文件在此)
├── include/                # 头文件 (.h) - 定义类接口与成员变量
│   ├── Blob.h              # 文件内容快照封装
│   ├── Commit.h            # 提交节点定义
│   ├── Index.h             # 暂存区管理
│   ├── Repository.h        # 核心仓库逻辑与命令分发
│   ├── Utils.h             # 文件读写、哈希计算、路径处理工具
│   └── GitliteException.h  # 异常处理
├── src/                    # 源文件 (.cpp) - 业务逻辑实现
│   ├── Blob.cpp
│   ├── Commit.cpp
│   ├── Index.cpp
│   ├── Repository.cpp
│   ├── Utils.cpp
│   └── GitliteException.cpp
├── testing/                # 自动化测试脚本与测试用例
├── CMakeLists.txt          # CMake 构建配置文件
├── main.cpp                # 程序入口 (解析命令行参数)
└── README.md               # 项目说明文档
```

其实现的功能如下

1. 基本功能：
```
./gitlite init                     
./gitlite add [filename]         
./gitlite commit [commit message]
./gitlite rm [filename]
./gitlite log                        # 当前分支的提交日志
./gitlite global-log                 # 包含所有分支提交的日志
./gitlite find [target message]      # 列举包含给定提交信息的提交
./gitlite status                     # 显示当前工作状态
```

2. 分支操作：
```
./gitlite checkout [branch]          # 以指定分支覆盖当前工作目录
./gitlite checkout [file]            # 将 HEAD 提交中的 file 版本覆盖工作目录
./gitlite checkout [commitid] [file] # 将对应提交的 file 覆盖
./gitlite branch [name]              # 建一个指定名称的新分支
./gitlite rm-branch [name]           # 删除指定名称的分支
./gitlite reset [commit_id]          # 重置当前分支的 HEAD 指针至指定提交
./gitlite merge [branch]             # 将指定分支的文件合并到当前分支
```

3. 远程操作：
```
./gitlite add-remote [name] [branch] # 将指定的外部地址保存为远程仓库
./gitlite rm-remote [name]           # 删除与给定远程名称关联的信息
./gitlite push [name] [branch]       # 将当前分支的提交附加至指定远程分支
./gitlite fetch [name] [branch]      #   
./gitlite pull [name] [branch]       # fetch 对应分支后与当前分支合并
```
 
相比 `Git` 的简化主要为
1. 没有处理工作目录为文件树的情况
2. 将远程仓库简化为非工作目录的本地文件夹
3. 实现主要面向泛文本文件
## 2. 持久化实现

`GitLite` 的状态完全保存在工作根目录下的 `.gitlite` 文件夹中。程序退出后，内存释放，但状态通过以下文件结构持久化：

```
.gitlite/
├── blobs/             
│   ├── a1b2c3...          # Blob 对象 (文件名为 SHA-1)
│   └── ...
├── commits/           
│   ├── a1b2c3...          # Commit 对象 (文件名为 SHA-1)
│   └── ...
├── branches/            
│   ├── Branchname 1       # 维护 branch 头指针对应的 Commit 
│   └── ...
├── HEAD                   # 全局头指针 (记录当前所在的分支名)
├── index/                 # 暂存区
│   ├── add                # 记录 Staged for addition 的文件列表
│   └── remove             # 记录 Staged for removal 的文件列表
└── remote                 # 记录 Remote 仓库的别名与路径映射
    ├── Remotename 1       # 维护对应的远程仓库地址 (/.gitlite)
    └── ...
```
其中具体的类与序列化 / 反序列化逻辑将在下面进行介绍

## 3. 类的设计与功能

### `Repository` 

Repository 是核心控制器对象，它建立在在 `.gitlite` 文件夹上维护的数据上，而`Gitlite` 直接通过调用 `Repository` 实例不同的成员函数实现不同指令，这些函数是通过与其他对象的交互实现的。
#### 实例函数

即每条指令所对应的函数，如 `init, add, commit, checkout, merge`等，分别实现各自功能。
#### 静态函数

1. 获取不同对象保存目录的函数 `getxxxDir`：`xxx` 包括 commit, blob, index 等。默认情况下基于 `getcwd` 在当前文件夹下组合目标对象相对路径而成，如果填入相对路径参数则以其为基础进行组合。这方便了不同类快速获取当前文件夹路径，并避免了硬编码路径值
2. 对 `Head` 指针进行处理的函数 `getHeadBranch`，`getHeadHash`，`getHeadCommit`，`rewritehead`，前三个函数分别获取头指针对应的分支，该分支指向的提交哈希值，与其提交对象本身，最后一个函数实现对头指针指向的分支进行重写
3. 对分支进行操作的函数 `getBranchHash`，`getBranchCommit`，逻辑与上一条类似，这两组静态函数处理了未抽象为类的头指针和分支。
4. 实现直接将一个路径下的某文件按二进制复制到目标路径的函数 `copyObject`（在目标路径去重）：这主要是为 `remote` 的处理设计的
### `Commit`

`Commit` 表示版本控制历史图中的一个提交节点
#### 数据成员

包括父提交和 merge 下存在的第二个父提交的哈希值 `father_hash, second_father_hash`，与其本身的 `Hash`，`message`，`time_stamp`，以及文件 - 内容映射 `file_blob_map`
#### 实例函数

- 传入父提交哈希值和提交信息的构造函数，默认情况下映射与父提交相同，基于暂存区的更新由 `Repository` 实现
- `save_commit`：调用 `commit_serial` 函数对 `commit` 进行序列化，计算哈希值后将提交内容写入 `.gitlite/commits`，并更新头指针
- `check_map`：返回文件 - 内容映射的一个引用
- `show`：将提交进行格式化打印输出
#### 静态函数

- `commit_serial`：将 `commit` 对象根据固定格式进行序列化
- `commit_deserial`：基于传入的哈希值获取 `.gitlite/commits` 中的序列化提交数据，随后根据 `serial` 的逻辑进行反序列化，输出对应的`commit` 实例
- `lowest_common_ancestor`：主要在 `merge` 函数中使用，计算给定的两个提交的最新公共祖先，基于多源 `BFS` 算法获取
### `Blob`

`Blob` 是用于保存文件内容快照的一个抽象类（即 `Binary Large Object`）
#### 数据成员

包括文件名称 `target_file`，创建时立即保存的 `content` 与只由 `content` 计算得到的哈希值 `Hash`
#### 实例函数

- 传入目标文件名的构造函数：在工作目录中找到对应文件并读取为序列化形式，计算哈希值
- `save_blob`：将 `blob` 的 `content` 写入 `./gitlite/blobs` 文件夹（保存后的文件与文件名无关，这是为了）
#### 静态函数

- `blob_deserial_content`：与 `commit_deserial` 函数类似，基于传入的哈希值获取 `.gitlite/blobs` 中的对应内容并直接输出，这与基于文件名的构造函数有所区分。

### `Index`

`Index` 表示整个暂存区结构，其实例通过读取硬盘初始化，通过写入硬盘实现持久化更新
#### 数据成员

- `added`：表示暂存区的添加区，维护文件 - 内容快照映射
- `removed`：表示暂存区的标记删除区，维护标记删除的文件名集合
#### 实例函数

- 默认构造函数：初始化时 `added` 与 `removed` 均置空，等待 `readFromDisk` 读取 / 外部对 `added` 与 `removed` 进行处理
- `writeToDisk`：将当前 `Index` 实例的添加区与标记删除区分别写入 `.gitlite/Index/added` 与 `.gitlite/Index/remove` 并覆盖原本的内容
- `readFromDisk`：基于 `writeToDisk` 的序列化逻辑进行反序列化并写入 `added` 与 `removed`

> `Index` 类没有专门设计添加 / 删除文件的接口，以直接对 `added` 和 `removed` 的内容进行修改的方式编辑暂存区内容，这相对更加灵活且对稳健性影响不大
### 其他

- 作业本身提供了包含大量工具函数的 `Utils` 和定义项目异常的 `GitliteException`  
- 在作业提供的 `Utils` 中增加了一个基于 `time_t` 格式时间戳计算格式化的本地时间的辅助函数 `format_time`

## 4. 安装与运行

本项目已实现 `CmakeLists`，可以通过

```
mkdir build && cd build
cmake ..
make
```

进行编译生成 `gitlite` 可执行项目，其可在 `build` 文件夹中直接测试并运行