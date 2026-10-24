# cleanup handoff — audit_only

独立交付完成，等监督者转交，无跨聊天消息。仅写 cleanup/；未启动引擎、未改资产/源码/配置，未提交推送。

- baseline.json：新组件文件出现前捕获 583 个旧资产（208,346,908 字节）及 SHA-256；09:56:52 +08:00 再验所有哈希未变。
- exact-six-current-check.json：六旧路径仍不存在；六 canonical 及原12备份哈希全部匹配。不重清。
- asset-decisions.json/csv：42 字面引用保留、15 动态拼接路径保留、525 兼容目标保留、1 Unknown 保留（M_InkLeaderboard 有23个MI二进制线索）。删除候选=0。
- static-text-evidence.json / binary-string-leads.json / dynamic-load-sites.json / known-dynamic-paths.json：399 文本和1498资产包扫描，无读取错误。静态线索不等于完整注册表或运行验收。
- audit_legacy.py：baseline/audit/精确名单 copy-and-hash backup，绝无删除命令。
- registry_audit_ue.py：完整 mounted roots 的五类引用检查，未实际执行。必须 supervisor lease：owner_team=cleanup、state=granted|active、cleanup_state=registry_audit、有效带时区 expires_at。
- verify_offline.py 已执行，offline-verification.json：7通过/0失败；未授权 registry gate 在 import unreal 前拒绝；真实样本备份hash通过。

复核命令（项目根）：
`& 'C:/Users/zxxuh/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' 'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/cleanup/verify_offline.py'`

下一步：导入/功能接入稳定且唯一引擎执行者空闲后，监督者授只读 registry lease，按 README 执行 registry_audit_ue.py；核对完整包覆盖、漂移、五类引用及动态/AssetManager管理来源后更新逐项决策。当前 lease 不满足，不能启动。仍无零用途证明则继续保留，不需要为了完成率找资源删。真正删除需另一个精确范围清理 lease，当前没有候选。

source_ready=true 指审计工具；import_verified/route_wired/logic_verified 均无本队完成声明；视觉保持用户审批、候选 approved:false，本队不修改候选状态。
