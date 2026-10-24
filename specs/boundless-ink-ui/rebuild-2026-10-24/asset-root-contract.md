# /Game/newUI 运行根与迁移协作契约

生产Source/Config：logic-integration。资产映射/备份/重命名/引用修复/旧目录与Registry归零：cleanup。当前用户Editor打开，两队仅准备，未授引擎前不执行Content操作。

## Canonical 路径

- 新默认运行WBP：`/Game/newUI/Runtime/WBP_NewUIFrontend`，Parent `UInkRebuildFrontendWidget`。
- 新运行组件/纹理建议：`/Game/newUI/Components/{Player,RankWuling,Journey,Activities}/...`；公共主题/字体与控件可在`/Game/newUI/Shared/`。
- 现有仍需资源可按明确映射将`/Game/UI/<relative>`迁到`/Game/newUI/<relative>`，保留相对层级以减少碰撞和引用歧义。这只是资产迁移，不是旧页面外观重新默认启用。
- `/Game/newUI/Runtime/`保留给全新Root/WBP，不与原`/Game/UI/Ink/...`的相对映射重名。cleanup发现碰撞须在执行前列出。
- 新源码常量/软引用/配置应使用newUI；旧`/Game/UI`只允许位于明确历史兼容alias表或旧证据，不能作为默认加载路径或丢失新资源时的fallback。

## 需要保留迁移的已有资源

字体、物品/品质/头像/角色表现、Niagara/材质及仍需的候选透明组件均按真实引用审计；未证明无用途不删除。现有WBP可以归档/迁移以回滚；默认玩家入口另切全新Runtime WBP，不用旧WBP主体加纸皮。

cleanup请提供稳定JSON映射：`source_package`、`target_package`、原始uasset/uexp/ubulk绝对路径和hash、目标碰撞状态、referencer/registry审计范围。需要C++helper时给明确输入schema和UE API要求，integration在Editor模块实现受限commandlet，不调用全项目globalFixup。

## 串行引擎阶段

1. Content外完整备份（Saved/Codex/UIRebuild20261024/Backups），再确认当前映射/碰撞/磁盘与registry一致。
2. 精确UE RenameAssets/保存referencers；不直接文件系统移动uasset。
3. 精确旧路径redirector修复，禁止EditorAssetLibrary.delete_asset旧redirector，禁止无边界globalFixup。
4. 导入新组件和保存新Root/页面WBP；Source canonical引用与迁移后的包实际匹配。
5. 新进程核对：Content/UI不存在，AssetRegistry `/Game/UI`包0，默认类/WidgetTree/纹理路径均newUI，入口/返回/数据/动作与截图通过。

迁移工具与路径改写的源代码可在用户Editor打开时准备；所有资产操作、构建/热更与引擎执行等现场协调。新外观交付与无旧资源根是两个独立必须证明的结果。
