# 提交规则

## 提交范围

- 提交本项目运行、构建或维护所需的源码、配置、插件源码、Unreal 资源、原始素材、设计文档和规格文件。
- 提交前确认文件属于本项目，且不是其他项目的副本、个人工作区数据或临时交付物。
- 不提交 Unreal 生成目录（`Binaries`、`Intermediate`、`DerivedDataCache`、`Saved`）、编辑器本机状态、IDE 生成的解决方案、编译产物、Python 字节码、分析报告、备份副本或操作系统元数据；这些路径由 `.gitignore` 过滤。
- 不提交密钥、令牌、签名文件或本机 `.env` 配置。需要公开配置样例时，只提交去除真实凭据的 `.env.example`。
- 大型 Unreal 资源和二进制素材由 Git LFS 管理。克隆仓库后先安装 Git LFS，再检出资源。

## 提交信息

每个提交聚焦一个可说明的变更，格式为：

```text
<type>(<scope>): <简短说明>
```

`type` 使用 `feat`、`fix`、`ui`、`refactor`、`assets`、`docs`、`build` 或 `chore`。`scope` 写受影响的系统、模块或资源范围；跨模块时可省略。说明使用简短中文或英文，明确表达变更内容，不写“更新”“修改”等空泛描述。

示例：

```text
feat(cultivation): 增加突破失败反馈
fix(inventory): 修复满包时物品丢失
assets(ui): 更新竖屏主页组件
docs(gameplay): 补充天劫规则
build(git): 添加 Unreal 资源 LFS 规则
```

## 提交前检查

1. 使用 `git status` 检查暂存范围，只暂存本次变更相关文件。
2. 使用 `git diff --cached --stat` 和 `git diff --cached --name-only` 复核提交内容；确认没有缓存、构建输出、凭据或无关文件。
3. 确认新增的大型二进制文件按 `.gitattributes` 进入 Git LFS。
4. 按变更类型运行适用的构建或验证，并在提交说明或评审中准确记录结果。

## Git LFS

仓库使用 Git LFS 管理 `.uasset`、`.umap`、`.uexp`、`.ubulk`、常见源素材和 ZIP 文件。新环境执行：

```powershell
git lfs install
git lfs pull
```
