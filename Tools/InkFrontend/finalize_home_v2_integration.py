"""Publish actual measured integration status after post-cleanup verification."""
from pathlib import Path
import json,yaml
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Saved/Codex/HomeV2Integration20261022';SPEC=ROOT/'specs/boundless-ink-ui'
def read(p):return json.loads(p.read_text('utf-8-sig'))
def write(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2),'utf-8')
proof=read(OUT/'delivery-validation.json');assert proof['passed']
pie=read(OUT/'PIE/report.json');regression=read(OUT/'PostCleanupPIE/report.json');assert regression['passed']
native=read(OUT/'NativeRegressionFinal/index.json');assert native['failed']==0 and native['succeeded']>0
cleanup=read(OUT/'cleanup-result.json');archive=cleanup['archive']
status=read(SPEC/'evidence/delivery-status.json')
status.update(stage='home-v2-v4-ue-integrated-cleanup-regression-complete',stage_complete=True,derivative_user_approved=True,runtime_files_written=True,ue_import_performed=True,runtime_tests_run=True,approval_evidence='evidence/approval-ue-integration-v2-v4.json',runtime_evidence='../../Saved/Codex/HomeV2Integration20261022/delivery-validation.json',cleanup_index='evidence/history-archive-index.json',remaining=[],old_packages_preserved='verified-external-archive',follow_up_outside_current_scope=['Cook not run','Android/iOS device verification not run','Production provider/platform approval not run','Other page artwork not delivered in this batch'])
status['visual_review']['user_approved']=True
status['package'].update(user_approved=True,status='user-approved-immutable-design-snapshot',metadata_snapshot='Original pre-integration metadata bytes retained; current runtime status is this delivery-status.json')
status['ue_runtime']=dict(entry='Existing map InkMenuDirector -> /Game/UI/Ink/Widgets/WBP_InkFrontend -> native HUD',textures='/Game/UI/Ink/HomeV2/Textures',material='/Game/UI/Ink/HomeV2/Materials/M_HomeAvatarCircle',pie_checks=len(pie['checks']),post_cleanup_checks=len(regression['checks']),screenshots=len(regression['captures']),native_passed=native['succeeded'],native_warnings=native['succeededWithWarnings'],native_failed=native['failed'])
status['cleanup']=dict(archived=len(cleanup['items']),bytes=sum(x['bytes'] for x in cleanup['items']),uasset_removed=0,archive=archive,pending_assets=proof['pending_assets'],pending_list='../../Saved/Codex/HomeV2Integration20261022/pending-cleanup.csv')
write(SPEC/'evidence/delivery-status.json',status)
for rel in ['contracts/sprite-contract.yaml','assets/sprites/home-components-trim-v2/sprite-manifest.yaml']:
 p=SPEC/rel;d=yaml.safe_load(p.read_text('utf-8'));d['user_approved']=True;d['integration_approval']='evidence/approval-ue-integration-v2-v4.json';d['runtime_status']='integrated-and-Windows-PIE-verified';p.write_text(yaml.safe_dump(d,allow_unicode=True,sort_keys=False,width=110),'utf-8')
p=SPEC/'contracts/asset-manifest.yaml';d=yaml.safe_load(p.read_text('utf-8'))
for a in d['assets']:
 if a['id']=='home-components-trim-v2-package':a.update(status='approved',approved=True,approval_evidence='evidence/approval-ue-integration-v2-v4.json')
p.write_text(yaml.safe_dump(d,allow_unicode=True,sort_keys=False,width=110),'utf-8')
body=f'''# 首页组件 UE 实际接入与清理交付

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
- 最终Windows PIE：{len(pie['checks'])}/{len(pie['checks'])}；清理后重跑：{len(regression['checks'])}/{len(regression['checks'])}，{len(regression['captures'])}张视口截图，并保留对应完整窗口原始截图。720×1280与1280×720，含实际Slate点击、实际页面断言、关闭、比例、SafeZone、数值绑定和头像遮罩。
- 原生回归：{native['succeeded']}通过，{native['succeededWithWarnings']}带警告通过，0失败。警告细节保留在NativeRegressionFinal/index.json，不把日志描述为零警告。
- UI资产硬/软/管理引用及动态源码路径扫描570项，重定向器0。保留相关打包规则。
- 保护性哈希与外部可恢复基线检查见delivery-validation.json。地图、相机、剑灵与无关玩法代码未改。
- Cook、Android/iOS设备、生产直播头像链路及平台审核均未执行。

完整窗口截图用于避开Slate子树重绘缓存的间歇缺块；视口PNG是按引擎实际几何从原始窗口像素裁出的区域，不重绘或修补内容。早期子树截图仅保留作诊断历史。

## 归档、清理与恢复

本次归档移除8份工作副本：2个已被替代的ZIP、5张v2失败预览及1张v3失败预览，合计{proof['cleanup_bytes']:,}字节。当前18张源组件、18张裁切PNG、有效契约、批准、必要通过证据及v2包保留。

归档：{archive}。archive-manifest.json记录原路径、SHA256和原因；baseline/含1058份接入前文件，baseline-manifest.json记录哈希。复制后核对全部哈希、ZIP CRC，并实际运行restore.ps1向隔离目录恢复8份文件。

清理清单：Saved/Codex/HomeV2Integration20261022/cleanup-result.json。当前松散文件的历史资源路径已更新；不可变旧快照及ZIP内部原路径通过 evidence/history-archive-index.json映射到外部归档，未篡改旧快照字节。历史生成器已加防覆盖门禁，复现请在外部基线的独立副本进行。

本次没有确认可安全移除的uasset，因此uasset删除数为0，没有从文件系统删除UE资产。其余{proof['pending_assets']}项相关旧/共享UI资源列在pending-cleanup.csv，说明引用、动态加载或用途未完全确认的保留理由。

仅恢复归档文件：运行归档目录的 restore.ps1。现有文件默认保留；需要覆盖时显式加 -Overwrite。回滚接入前基线：关闭UE，运行 restore.ps1 -RestoreBaseline -Overwrite，再使用本机引擎绝对路径 Build.bat重编译。新HomeV2资产可保留为未使用资源；若要彻底移除，应另经编辑器资产工具和引用复核，不直接删uasset。

## 证据与续接

Saved/Codex/HomeV2Integration20261022/progress.json、delivery-validation.json、PIE/report.json、PostCleanupPIE/report.json、visual-review.json、ue-reference-audit.json、NativeRegressionFinal/index.json。

设计包严格校验只证明设计文件；编译、PIE、引用扫描、Cook和设备验证分别记录。原v2 ZIP哈希178851e9e1e935243216b57509af5d19c30863593c60a2c3964263f7642cafcb保持不变，内部状态是设计阶段快照；当前状态以evidence/delivery-status.json和本次批准为准。
'''
(OUT/'handoff.md').write_text(body,'utf-8');(SPEC/'ue-integration-handoff.md').write_text(body,'utf-8')
(SPEC/'component-handoff.md').write_text(body,'utf-8')
(SPEC/'quickstart.md').write_text('# 当前阶段：UE已接入并完成归档回归\n\n见[实际交付说明](ue-integration-handoff.md)。批准见evidence/approval-ue-integration-v2-v4.json；可恢复归档见evidence/history-archive-index.json。\n\n验证命令：本机gameui Python运行 Tools/InkFrontend/verify_home_v2_delivery.py；设计包另运行validate_project.py --strict。历史纯设计生成器及旧校验器描述的是接入前快照，复现请使用外部baseline独立副本。\n\n字体、源PNG与当前v2设计ZIP保持；原ZIP内历史状态不是当前运行状态。Cook/设备/生产验证未执行。\n','utf-8')
(SPEC/'README.md').write_text('# 无极仙道首页组件实际接入\n\n[UE实际接入、截图、清理及恢复](ue-integration-handoff.md)\n\n当前状态：evidence/delivery-status.json；批准：evidence/approval-ue-integration-v2-v4.json；历史迁移：evidence/history-archive-index.json。当前渲染契约 contracts/sprite-contract.yaml 与 assets/sprites/home-components-trim-v2/sprite-manifest.yaml。\n\n本批仅首页组件。其他页沿用原运行实现，不把页面规划当成已交付切图。\n','utf-8')
(SPEC/'tasks.md').write_text('''# 本批实际任务
- [x] 核对现行v2/v4与批准；复用源图和审核证据。
- [x] 1058份文件备份及SHA256核对，外部可恢复基线。
- [x] 18纹理及头像材质导入，现有玩家HUD实际接入。
- [x] 原生构建、资源加载、横竖Windows PIE、真实点击及关闭导航。
- [x] Fit、锁高切片、排行条件切换、真实fraction及头像遮罩。
- [x] UI引用/配置/打包/动态源码路径/设计证据审计。
- [x] 8份确定废弃资源先外部归档、哈希/CRC/恢复演练，再移除工作副本和更新引用。
- [x] 清理后实际PIE、原生回归与保护哈希核验。
- [x] 不明用途和仍被使用的UE资源保留，记录pending-cleanup.csv。
- [ ] Cook、Android/iOS设备、生产provider和平台审核：未执行，超出本批通过边界。
- [ ] 其他页面新增切图：未交付，保留规划状态。
''','utf-8')
p=OUT/'progress.json';d=read(p);d.update(stage='complete',stages={k:'passed' for k in ['backup','integration','runtime_validation','cleanup','regression']},final_handoff=str(OUT/'handoff.md'),validation=str(OUT/'delivery-validation.json'),archive=archive,pending_cleanup_assets=proof['pending_assets']);write(p,d)
print('Final integration metadata and handoff updated from actual evidence')
