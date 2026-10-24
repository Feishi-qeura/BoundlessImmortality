# 新版 UI 全面接入与本地验证

## 已接入的运行界面

宣纸底、青玉选中态、动态文字与真实空态已接入主菜单、存档/设置、角色、背包、三类榜单、武灵、世界/资源地图、签到、常驻/日常/限时/开服活动、突破/天劫及关联工具页。已有保存 WBP 在运行时迁移对应子树。图标、头像、地图等仍在使用的美术保留；候选整页图片没有冒充可交互页面或真实数据。

本轮修正了实际 PIE 截图中的背包整体缩小、榜单列头重叠与空榜填充、签到双重滚动和无日期伪日历、武灵空图框、开服状态标题贴边等问题。缺失离线字段不再显示为0；未经核实的费用比例和时限不再由UI推算。预览外观文字增加纸底，存档/设置选中态与禁用态文字修正对比度。

## 旧 UI 清理

精确清理了6个旧 ObjectRedirector 包。同步 Asset Registry 全量扫描、目标身份、唯一正式目标、零磁盘入向引用及12包备份均通过后，才在空闲 GUI Editor 中调用一次窄范围 FixupReferencers。正式6包的 SHA-256 保持不变，569条配置与C++别名保持一致。

- [清理结果](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/cleanup-result.json)
- [交付前再次核验](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/delivery-assets-check.json)
- [12包备份清单](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/precleanup-backup.json)

不删除仍在使用或引用情况未证实的其他美术。备份保留在同目录的 precleanup-backup/。

## 本地验证

| 检查 | 结果 | 证据 |
|---|---|---|
| Editor Development 构建 | 修正批次20/20、可读性增量5/5及最终7/7 actions，退出码0 | visual-fixes-build.log、final-contrast-build.log、delivery-build.log |
| 最终 Frontend 自动化 | 40通过 + 1带警告通过，0失败/未运行 | verified-frontend-tests/index.json |
| 清理模式解析 | 1通过，拒绝重复/双模式/未知参数 | cleanup-mode-tests/index.json |
| GameUI strict | 0 warning | validate_project.py --strict |
| 正式资源 | 旧6包不存在，正式6包哈希保持 | delivery-assets-check.json |
| PIE组件预览 | 44/44路由完成，0失败，88张PNG；进程退出码0 | visual-suite-20261024T010041Z-A0CAA6DE/report.json、verified-visual-process.json |
| 玩家入口 | 主菜单启动、存档/设置Slate点击及返回3项通过，41项受限未执行，0失败；进程退出码0 | BoundlessInkPIE/20261024T005938Z-3DC85B0E/report.json、verified-navigation-process.json |

自动化中唯一警告来自 StageCoordinatesKeepPawnRoles 在隔离临时World里销毁角色时没有context。引擎启动还有自身 UnifiedErrorTest 示例日志，不能宣称日志全无Error；Frontend报告的失败数为0。

第一轮截图工具结束时同帧结束PIE并退出Editor，引发PCG/Water关闭异常。现已等待PIE完整销毁再退出；第二轮与最终轮均44路由截图完成且进程退出码0。最终预览和导航日志的内存异常、Critical error、ensure及退出超时匹配数均为0；测试后没有遗留Editor/构建实例。

入口检查曾误报存档/设置回调缺失：工具在同步Slate点击之后才记下回调计数基线。已把基线移至点击前；复跑确认两页都通过真实点击打开并返回，未通过程序直达页面替代。41项受限路线包括需要StartGame、服务或当前未显示入口的路线，不计为通过。

## 验证边界

组件预览直接调用公开 ShowPage/OpenFeaturePanel，逐条核对页面、类、组选中状态并截图；它不证明玩家逐入口可达。44个契约路由包含共用面板与别名，不等于44个独立业务状态。武灵和修炼服务均关闭，未执行签到领奖、购买、突破、存档写入等操作；有数据的业务结算、设备及平台验收仍需相应环境。

PIE请求的游戏分辨率为720×1280；取景widget的PNG实测为720×1282，整窗PNG为726×1320。文件名沿用请求尺寸，不能把它当作像素完全一致的设备截图。

新DLL身份见 [final-binaries.json](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/final-binaries.json)。重新打开工程会加载该构建。

汇总与源码指纹见 [final-verification.json](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/final-verification.json)。

## 最终截图示例

- [背包](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/visual-suite-20261024T010041Z-A0CAA6DE/04-inventory-720x1280.png)
- [排行榜](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/visual-suite-20261024T010041Z-A0CAA6DE/05-leaderboard-720x1280.png)
- [月签离线空态](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/visual-suite-20261024T010041Z-A0CAA6DE/26-monthly-sign-in-720x1280.png)
- [外观预览](D:/demo/BoundlessImmortality/Saved/Codex/FullUI20261024/visual-suite-20261024T010041Z-A0CAA6DE/42-appearance-720x1280.png)
