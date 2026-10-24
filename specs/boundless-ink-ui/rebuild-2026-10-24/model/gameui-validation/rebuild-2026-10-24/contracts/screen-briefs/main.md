# 主菜单 · main / newui-r1

Owner: panel-player; 合并片: player-a; 状态: contract_ready / approved:false / visual_pending_user。

目标：看清开始与继续，人物场景成为新版主视觉。新 PageKey=main；兼容入口=Main，目前源码解析到 Main。当前类只是业务事实，不要求复用旧可见树。新实际class/WBP尚未核实；依architecture由integration创建真实新页宿主，保留新root与页面标记。

构图族：scene-entry。竖屏采用 style-contract 的720×1280页框/字级/24安全边距/88触控目标；横屏重新排列表/详情，不缩放整页。必备组件：page-frame, state-panel, back-button, main-scene-window, primary-button, secondary-button。

视觉来源：player-main-v3（source-registry.json）；有对应候选作为布局输入。禁止全页贴图，动态文案与操作分层。

数据模型：FrontendVm, SaveVm。操作：start-game, continue-game, open-saves, open-settings, quit-game，精确映射见 model/action-contract.yaml。Unknown/loading/empty/data/pending/offline/error/disabled分别演示；仅展示缺字段不阻断其它合法动作。

特定变体：单页；按服务实际状态呈现。缺口：未确认的新字段继续unknown；不得为版面补数。

执行交付：1) 独立新页面/组件树；2) 真实VM字段和事件slot；3) 绑定到已有业务处理和新版宿主；4) 仅newUI运行纹理/字体与新布局版本；5) portrait/landscape实际入口；6) 数据/动作与截图各自记证据。局部slot、纸框、PNG、静态编译均不能置page_composed/player_entry_verified=true。
