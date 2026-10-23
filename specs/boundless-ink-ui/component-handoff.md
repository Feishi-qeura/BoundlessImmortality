# 首页组件 UE 实际接入与清理交付

2026-10-22，Asia/Hong_Kong。首页组件切图 v2 与渲染策略 v4 已按本次批准接入现有运行界面。

## 实际入口与资源

- 原有地图中的 InkMenuDirector → /Game/UI/Ink/Widgets/WBP_InkFrontend → InkHudLayout.cpp、InkAnchorStatusWidget.cpp 和 HUDRankView。没有另建演示页，没有重存地图或角色/背包蓝图。
- 18张纹理：/Game/UI/Ink/HomeV2/Textures；真实头像圆形遮罩：/Game/UI/Ink/HomeV2/Materials/M_HomeAvatarCircle。
- 源PNG仍在设计包原路径 assets/sprites/home-components-trim-v1/items；目录名v1表示像素版本，当前v2清单继续引用，不能删除。
- 标题马善政体，正文与数字霞鹜文楷，保留现有缺字回退；原字体资产未覆盖。

## 已实现的显示与绑定

顶部底板、图标、头像框、境界牌、墨底、云带均保持比例；轨道按裁切尺寸2046×226和像素边距260/260/60/60计算，只横向伸缩。归一化Margin由像素除实际裁切宽高获得，绘制使用等价锁高三列，避免Box端帽尺度问题。

查看排行使用1575×399原图；容器窄于1575/399时整图Fit，宽时按820/400/115/125端区锁高横向切片。文字与真实点击几何都在Fit后的图形范围内。竖屏的88单位点击目标置于人物上方，横屏保留右侧位置。

血量与修为继续读取CultivationSubsystem及HealthComponent，fraction遮罩按Fit后的填充图形范围计算；保留第三条护盾与未激活状态。未知头像保留原占位，修士插图仅用于角色入口。本地GWLocalTest模式显示“视觉预览 · 本地模拟”；白方块头像仅是本次圆形遮罩测试输入，不代表实际主播头像。生产头像下载和服务接入不在本次通过范围。

角色属性、背包、主菜单、活动/签到、排行榜、菜单、修炼及关闭导航均沿用现有业务。本批其他页面没有新增设计切图，具体未交付项见 Saved/Codex/HomeV2Integration20261022/other-pages-pending.json。

## 实测结果

- 原生Editor构建通过，18张纹理和1个头像材质保存并实际加载。
- 最终Windows PIE：78/78；清理后重跑：98/98，15张视口截图，并保留对应完整窗口原始截图。720×1280与1280×720，含实际Slate点击、实际页面断言、关闭、比例、SafeZone、数值绑定和头像遮罩。
- 原生回归：93通过，1带警告通过，0失败。警告细节保留在NativeRegressionFinal/index.json，不把日志描述为零警告。
- UI资产硬/软/管理引用及动态源码路径扫描570项，重定向器0。保留相关打包规则。
- 保护性哈希与外部可恢复基线检查见delivery-validation.json。地图、相机、剑灵与无关玩法代码未改。
- Cook、Android/iOS设备、生产直播头像链路及平台审核均未执行。

完整窗口截图用于避开Slate子树重绘缓存的间歇缺块；视口PNG是按引擎实际几何从原始窗口像素裁出的区域，不重绘或修补内容。早期子树截图仅保留作诊断历史。

## 归档、清理与恢复

本次归档移除8份工作副本：2个已被替代的ZIP、5张v2失败预览及1张v3失败预览，合计52,213,566字节。当前18张源组件、18张裁切PNG、有效契约、批准、必要通过证据及v2包保留。

归档：D:\demo\BoundlessImmortality-UIArchive\20261022-home-v2。archive-manifest.json记录原路径、SHA256和原因；baseline/含1058份接入前文件，baseline-manifest.json记录哈希。复制后核对全部哈希、ZIP CRC，并实际运行restore.ps1向隔离目录恢复8份文件。

清理清单：Saved/Codex/HomeV2Integration20261022/cleanup-result.json。当前松散文件的历史资源路径已更新；不可变旧快照及ZIP内部原路径通过 evidence/history-archive-index.json映射到外部归档，未篡改旧快照字节。历史生成器已加防覆盖门禁，复现请在外部基线的独立副本进行。

本次没有确认可安全移除的uasset，因此uasset删除数为0，没有从文件系统删除UE资产。其余551项相关旧/共享UI资源列在pending-cleanup.csv，说明引用、动态加载或用途未完全确认的保留理由。

仅恢复归档文件：运行归档目录的 restore.ps1。现有文件默认保留；需要覆盖时显式加 -Overwrite。回滚接入前基线：关闭UE，运行 restore.ps1 -RestoreBaseline -Overwrite，再使用本机引擎绝对路径 Build.bat重编译。新HomeV2资产可保留为未使用资源；若要彻底移除，应另经编辑器资产工具和引用复核，不直接删uasset。

## 证据与续接

Saved/Codex/HomeV2Integration20261022/progress.json、delivery-validation.json、PIE/report.json、PostCleanupPIE/report.json、visual-review.json、ue-reference-audit.json、NativeRegressionFinal/index.json。

设计包严格校验只证明设计文件；编译、PIE、引用扫描、Cook和设备验证分别记录。原v2 ZIP哈希178851e9e1e935243216b57509af5d19c30863593c60a2c3964263f7642cafcb保持不变，内部状态是设计阶段快照；当前状态以evidence/delivery-status.json和本次批准为准。
