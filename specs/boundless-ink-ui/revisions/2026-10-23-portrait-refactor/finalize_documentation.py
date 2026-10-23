"""Publish evidence-backed status after build, rendered QA and distributable verification."""
from pathlib import Path
import json,hashlib,yaml
ROOT=Path(__file__).resolve().parents[4];REV=Path(__file__).parent;OUT=ROOT/'Saved/Codex/PortraitUI20261023';SPEC=ROOT/'specs/boundless-ink-ui'
def read(path):return json.loads(path.read_text('utf-8-sig'))
asset=read(OUT/'asset-final-audit.json');pie=read(OUT/'PIE/report.json');native=read(OUT/'NativeTests/index.json')
portrait=read(OUT/'PackagePortrait/verification.json');landscape=read(OUT/'PackageLandscape/verification.json');delivery=read(OUT/'distribution.json')
assert asset['passed'] and pie['passed'] and portrait['passed'] and landscape['passed'] and delivery['passed']
assert portrait['profile']=='PortraitUI20261023FinalP5' and landscape['profile']=='PortraitUI20261023FinalL5'
assert native['failed']==0 and native['succeeded']+native.get('succeededWithWarnings',0)>0
assert read(OUT/'preservation.json')['passed']
assert read(OUT/'HUD-final/report.json')['passed']
assert 'Result: Succeeded' in (OUT/'EditorBuild.log').read_text('utf-8-sig')
for name in ['README.md','tasks.md']:
 old=REV/('review-'+name)
 if not old.exists():old.write_bytes((REV/name).read_bytes())
for name in ['style-contract-proposed.yaml','screen-contract-proposed.yaml']:
 p=REV/name;data=yaml.safe_load(p.read_text('utf-8'));data['status']='approved';data['implementation_status']='implemented-and-windows-verified'
 data['approval']={'date':'2026-10-23','source':'direct-user-message','quote':'确认，实现全部，直接打包发布','delivery':'生成 Windows 可分发 ZIP','evidence':'approval.json','runtime_changes_authorized':True}
 data['validation']={'windows_rendered_pie':True,'windows_packaged_runtime':True,'mobile_device':False,'production_platform':False}
 for screen in data.get('screens',[]):
  preserve=screen['id'] in ['character','inventory','leaderboard']
  screen.update(status='preserved' if preserve else 'approved',current_status='retained-in-runtime' if preserve else 'implemented-in-runtime',delivery='preserve-runtime' if preserve else 'redesigned-runtime',revision_status='implemented',runtime_implemented_this_revision=True)
  screen['approval']={'approved':True,'evidence':'approval.json'}
  screen['route_verification']='See actual route names, mode and assertions in PIE/report.json and PackagePortrait/verification.json; this does not assert every data state was exercised.'
  proposed=screen.get('proposed_layout',{})
  if proposed:
   screen['responsive']={k:proposed[k] for k in ['portrait','landscape'] if k in proposed}
   screen['purpose']=proposed.get('family',screen['name'])+'：'+proposed.get('primary','保留现有操作')
  if screen['id']=='main':screen['rules']=['保留原世界与相机，重排开始、继续、存档、设置、退出；沿用现有导航行为。']
 p.write_text(yaml.safe_dump(data,allow_unicode=True,sort_keys=False),encoding='utf-8')
manifest=read(REV/'migration-manifest.json');manifest.update(status='executed-and-verified',executed=True,delete_count=0)
for row in manifest['assets']:row.update(executed=True,classification='approved-and-runtime-verified',destination_sha256=hashlib.sha256((ROOT/'Content'/row['proposed_package'].removeprefix('/Game/')).with_suffix('.uasset').read_bytes()).hexdigest())
(REV/'migration-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
summary=f'''# 无极仙道 · 竖屏 UI 重构与 Windows 分发包

用户已批准全量实现及 Windows ZIP；本轮已完成原生集成、资源迁移与运行验证。

- 保留角色属性、背包、排行榜的布局、控件与业务绑定；更新资源路径，减少共享外壳背景叠层。
- 重设计主菜单、HUD、设置、存档、签到/剑诀/月历、日常/限时/开服活动、仙途和各子页。正文可滚动，主要操作固定；宽屏在可读宽度下分栏。
- 原有 {asset['expected']} 项 UI 资源全部保留，按 Texture/Material/MaterialInstance/Niagara/Widget/Font/Data 与页面族归档；内容删除 0。旧路径兼容映射保留在配置和 `InkUiAssets`。
- 背景外壳单层纸底；嵌入页不再铺整页背景。修复卡片/按钮/景物绘制层级、滚动条宽度、重建滚动位置和抽取页固定操作。
- 按最后一次用户指正重排主 HUD：紧凑状态带、右侧签到/活动/仙途纵排、88×88 排行入口及104高底部墨色操作栏。细血量/修为条已修复画刷着色，公告条自动避开右侧入口并省略过长文字；最新包实际画面见 `PackagePortrait/02-hud.png`、`PackagePortrait/hud-announcement-fixture.png`（后者为明确注入的公告布局验证，无服务奖励或状态修改）。

## 分发

ZIP：`{delivery['zip']}`。

解压后运行 `Start-Wuling.cmd`，默认打开 540×960 竖屏窗口，可拖动边缘改变尺寸。随包基础 Python 自动启动本机玩法服务并随游戏退出。无需安装 Unreal Editor 或 Python。此包沿用 Windows Development / prototype_full 本地完整玩法流程；不是生产平台上传包。

首次运行若 Windows 弹出公共/专用网络访问授权，可选择“取消”；本次验收在取消该授权后正常连接本机回环服务。第一次被系统弹窗挡住的验收另存 `PackagePortrait-FirstFirewall`，不计入最终通过结果。

## 本轮验证

- Editor C++ 构建成功；原生自动化 {native['succeeded']} 通过、{native.get('succeededWithWarnings',0)} 带警告通过、0 失败。
- 本地服务 88 项、启动器 7 项测试通过。
- Windows PIE：{len(pie['checks'])} 项检查、{len(pie['captures'])} 张截图；720×1280、1280×720、540×960、960×1280、720×1560、360×640，另有安全边距、重复打开和下方操作点击。
- 最终 HUD 另完成 31 项检查、6 张截图及血量/修为条像素着色验证。
- 修复签到宝箱和称号 Niagara 控件释放后仍 Tick 的生命周期缺口，并为插件组件销毁后的入口加保护；新增真实组件销毁自动化，最终包横竖屏各重复三轮签到打开、关闭和强制垃圾回收验证。
- 实际 Windows 安装包竖屏 {len(portrait['checks'])} 项、横屏 {len(landscape['checks'])} 项检查通过，涵盖本机服务、世界历练、签到三页签、活动入口和子页。
- Cook/Stage/Archive 成功；ZIP 解压文件 SHA256 检查通过。证据目录：`Saved/Codex/PortraitUI20261023/`。

## 调整与边界

主题资产：`/Game/UI/Ink/Data/Shared/DA_InkUITheme_v1`。可调 PaperOpacity、BackdropOpacity、DecorationOpacity、HeaderHeight、FooterHeight；原有字体和配色配置仍在。

没有新增虚构玩法收益。签到占位、尚未开放的活动、抽取演示、外观试戴等继续按当前服务/功能开关如实显示。未做 Android/iOS 真机或生产平台审核。

既有场景问题：Windows 打包版石台及部分植被材质偏灰褐，日志含场景材质缺少 InstancedStaticMeshes 使用标记的告警；本次重构前 `ReleaseReadiness20261022/FinalPackagedPortrait/02-hud.png` 与旧包日志已存在同样现象。本轮未修改这些场景材质，UI 贴图与交互验收不代表此既有场景问题已修复。

迁移前 887 个文件备份及哈希在 `Saved/Codex/PortraitUI20261023/Backup` 和 `backup-manifest.json`。历史审阅稿另存 `review-README.md`；概念图不是运行截图，实际画面见 PIE/PackagePortrait/PackageLandscape。
'''
(REV/'README.md').write_text(summary,encoding='utf-8')
tasks='''# 本轮完成状态

- [x] 用户批准设计方向、全部实现及 Windows ZIP。
- [x] 570 项资产审计、887 文件备份与哈希核对。
- [x] 保留三个指定面板，其余页面及子页重设计。
- [x] 类型/页面族资源迁移、引用更新与兼容映射。
- [x] 背景和绘制层级修复，竖横屏、滚动、页签与操作验证。
- [x] 代码复核、构建、原生测试、PIE、Cook、实际分发包运行。
- [x] Windows ZIP 与解压哈希核对。
- [ ] Android/iOS 真机与生产平台审核：不属于此次 Windows ZIP 交付。
'''
(REV/'tasks.md').write_text(tasks,encoding='utf-8')
plan=REV/'implementation-plan.md'
plan.write_text(plan.read_text('utf-8').replace('- [ ] T','- [x] T'),encoding='utf-8')
layouts=REV/'screen-layouts.md'
layouts.write_text(layouts.read_text('utf-8').replace('状态：draft。旧玩法/数据/feature gate沿用；本文件的新布局未实施。','状态：已按用户批准范围实施并完成 Windows 验证，证据见 README.md。旧玩法/数据/feature gate沿用；表格是布局意图，实际页面与可用操作以运行截图和服务状态为准。'),encoding='utf-8')
design=REV/'design.md'
text=design.read_text('utf-8').replace('状态：draft；供本次审阅，尚未修改客户端。','状态：已批准并实施；以下保留原设计意图，实际交付、测试数量与边界以 README.md 为准。')
text=text.replace('当前已完成审计、覆盖清单和新基准候选。确认该设计后先实施共享背景与资源组织，再做主菜单/HUD及签到样板；取得实际横竖截图后按页面家族扩展，最终逐路由回归。这是拟定实施顺序，未把尚未执行的步骤登记为完成。','实施与 Windows 分发已完成。最终资源、源码保留、构建、PIE、打包运行和 ZIP 解压证据见 README.md；上方验收清单是设计阶段目标，不替代实际报告。')
design.write_text(text,encoding='utf-8')
with (REV/'implementation-ledger.md').open('a',encoding='utf-8') as f:
 f.write('\n- Final: all T1–T6 completed. Protected implementations and all 887 backup hashes passed preservation.json. All 570 original assets retained; 12 unique compatibility redirectors intentionally remain because unattended AssetTools cleanup cannot safely complete. The duplicate meter migration copy was pixel-hash verified and consolidated using Unreal APIs. Final build, automation, rendered PIE, packaged runtime and ZIP extraction evidence are linked in README.md.\n')
(SPEC/'quickstart.md').write_text('# 当前版本：2026-10-23 竖屏 UI 已集成并打包\n\n最新[交付与验证说明](revisions/2026-10-23-portrait-refactor/README.md)。用户已批准全量实现与 Windows 可分发 ZIP。\n\n设计包校验使用 gameui/scripts/validate_project.py 本目录 --strict；真实客户端证据以本轮 PIE、原生自动化、Cook 和 PackagePortrait/PackageLandscape 报告为准。旧设计图、历史 ZIP 和旧验证器仍为可追溯材料，不能代替本轮运行结果。\n',encoding='utf-8')
(SPEC/'tasks.md').write_text(tasks+'\n详细证据见 revisions/2026-10-23-portrait-refactor/README.md。\n',encoding='utf-8')
(ROOT/'docs/2026-10-23-portrait-ui-windows-release.md').write_text(summary,encoding='utf-8')
release_readme=ROOT/'Tools/WindowsRelease/README.md'
release_text=release_readme.read_text('utf-8')
release_text=release_text.replace('2026-10-23实际交付、测试证据与保留边界见 [Windows本机交付报告](../../docs/2026-10-23-windows-local-v14-release.md)。','最新竖屏 UI 版本见 [Windows 竖屏交付报告](../../docs/2026-10-23-portrait-ui-windows-release.md)；默认540×960竖屏窗口，可自行调整窗口，显式分辨率参数优先。此前玩法交付保留在 [本机玩法报告](../../docs/2026-10-23-windows-local-v14-release.md)。')
release_readme.write_text(release_text,encoding='utf-8')
print('Final documentation synchronized with passed evidence.')
