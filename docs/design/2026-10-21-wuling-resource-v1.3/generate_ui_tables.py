"""Author/verify planning tables. Does not create runtime code or Unreal assets."""
from pathlib import Path
import csv
import hashlib
import json
from PIL import Image

ROOT = Path(__file__).resolve().parent
PROJECT = Path(r"D:\demo\BoundlessImmortality")
TABLES = ROOT / "tables"
BP = "Content/UI/Ink/Wuling/Widgets/"
CP = "Source/BoundlessImmortality/Frontend/"


def save_csv(name, rows):
    with (TABLES / name).open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def main():
    screens = []

    def screen(sid, route, title, current, native, asset, parent, phase, gate, vm, action, surface="anchor_ue"):
        screens.append(dict(screen_id=sid, route=route, title=title, current_status=current, native_class=native, widget_asset=asset, parent=parent, phase=phase, feature_gate=gate, view_model=vm, primary_action=action, surface=surface))

    legacy = "Content/UI/Ink/Widgets/"
    for sid, route, title, asset, vm, action in [
        ("UI_MAIN", "Main", "主菜单", "MainMenu", "FrontendVm", "开始或继续"),
        ("UI_HUD", "HUD", "常驻HUD", "Frontend", "AnchorVm|WorldVm|LeaseVm", "修炼收功及导航"),
        ("UI_CHARACTER", "Character", "角色", "Character", "AnchorVm", "主播突破"),
        ("UI_INVENTORY", "Inventory", "物品", "Inventory", "InventoryVm", "选中详情及允许动作"),
        ("UI_RANK", "Leaderboard", "排行", "Leaderboard", "LeaderboardVm", "切换真实榜单"),
        ("UI_SAVES", "Saves", "存档", "Saves", "SaveVm", "明确本地存档范围"),
        ("UI_SETTINGS", "Settings", "设置", "Settings", "SettingsVm", "保存允许设置"),
    ]:
        screen(sid, route, title, "existing_wbp_and_source_not_runtime_tested", "UInkFrontendWidget", legacy + "WBP_Ink" + asset + ".uasset", "UI_HUD" if route not in ("Main", "HUD") else "", "UI-P1", "economic_inventory" if route == "Inventory" else "base", vm, action)

    screen("UI_WULING", "Wuling", "武灵总览成员建设记录", "existing_native_prototype_calls", "UInkWulingPanel", BP + "WBP_InkWulingOverview.uasset", "UI_HUD", "UI-P1", "base", "GuildVm|WorldVm", "创建与成员管理")
    for sid, route, title, native, asset, parent, phase, gate, vm, action in [
        ("UI_SWORD", "SwordSpirit", "我的剑灵", "UInkSwordSpiritPanel", "SwordSpirit", "UI_WULING", "UI-P1", "base", "SwordVm|GiftEffectVm", "查看权益及选择称号"),
        ("UI_GIFT_RULES", "GiftRules", "礼物效果说明", "UInkGiftRulesPanel", "GiftRules", "UI_SWORD", "UI-P1", "base", "GiftEffectVm", "查看当前模式和期限"),
        ("UI_METHODS", "CultivationMethods", "功法构筑", "UInkMethodsPanel", "Methods", "UI_CHARACTER", "UI-P2", "base", "MethodVm", "准备期确认配置"),
        ("UI_RACE_LOBBY", "RaceLobby", "竞采匹配", "UInkRaceLobbyPanel", "RaceLobby", "UI_RESOURCE_DETAIL", "UI-P2", "cross_host_race", "RaceVm|GuildVm", "报名或取消"),
        ("UI_RACE_LIVE", "RaceLive", "竞采现场", "UInkRaceLivePanel", "RaceLive", "UI_RACE_LOBBY", "UI-P2", "cross_host_race", "RaceVm", "响应当前阶段"),
        ("UI_RACE_RESULT", "RaceSettlement", "竞采结算", "UInkRaceSettlementPanel", "RaceSettlement", "UI_RACE_LIVE", "UI-P2", "cross_host_race", "RaceVm|RewardVm", "确认展示及查看开采权"),
        ("UI_LEASE", "ResourceLease", "当前开采权", "UInkResourceLeasePanel", "ResourceLease", "UI_RACE_RESULT", "UI-P2", "cross_host_race", "LeaseVm", "回历练或共修"),
        ("UI_ROUND_RESULT", "EventSettlement", "历练结算", "UInkEventSettlementPanel", "EventSettlement", "UI_WORLD", "UI-P1", "base", "RewardVm", "确认展示"),
        ("UI_OFFLINE", "OfflineRewards", "离线收益", "UInkOfflineRewardsPanel", "OfflineRewards", "UI_HUD", "UI-P3", "offline_cultivation", "RewardVm", "确认已入账展示"),
        ("UI_SHOP", "Shop", "原型商店", "UInkShopPanel", "Shop", "UI_HUD", "UI-P3", "npc_shop", "EconomyVm|InventoryVm", "确认制作或购买"),
        ("UI_MARKET", "Market", "原型寄售", "UInkMarketPanel", "Market", "UI_HUD", "UI-P3", "player_trade", "EconomyVm|InventoryVm", "整单买入或挂牌"),
        ("UI_ORDERS", "TradeOrders", "我的订单", "UInkTradeOrdersPanel", "TradeOrders", "UI_MARKET", "UI-P3", "player_trade", "EconomyVm", "撤单与查看应收"),
    ]:
        screen(sid, route, title, "new_planned", native, BP + "WBP_Ink" + asset + ".uasset", parent, phase, gate, vm, action)

    for sid, route, title, native, current, parent, phase, gate, vm, action in [
        ("UI_WORLD", "WorldCultivation", "世界历练", "UInkWorldCultivationPanel", "existing_live_qi_plus_preview_regions", "UI_HUD", "UI-P1", "base", "WorldVm|AnchorVm", "选路线难度并开始"),
        ("UI_REGION", "RegionDetail", "区域详情", "UInkRegionDetailPanel", "existing_preview", "UI_WORLD", "UI-P1", "base", "WorldVm", "查看固定供给"),
        ("UI_RESOURCES", "ResourceContest", "资源争夺", "UInkResourceContestPanel", "existing_preview", "UI_HUD", "UI-P2", "cross_host_race", "LeaseVm|RaceVm", "选择五类资源"),
        ("UI_RESOURCE_DETAIL", "ResourceDetail", "资源详情", "UInkResourceDetailPanel", "existing_preview", "UI_RESOURCES", "UI-P2", "cross_host_race", "LeaseVm|RaceVm", "查看条件及进入匹配"),
        ("UI_SIGN", "SignIn", "签到组及日签到", "UInkSignInGroupPanel", "existing_preview", "UI_HUD", "UI-P3", "cross_session_numeric", "ActivityVm", "每日一次签到"),
        ("UI_MONTH", "MonthlySignIn", "月签到", "UInkMonthlySignInPanel", "existing_preview_old_7_14_21_28", "UI_SIGN", "UI-P3", "cross_session_numeric", "ActivityVm", "累计3_7_14_21节点"),
        ("UI_VERSE", "DailyFortune", "每日剑诀", "UInkDailyFortunePanel", "existing_demo_text", "UI_SIGN", "UI-P3", "base", "ActivityVm", "查看固定轮换剑诀"),
        ("UI_ACTIVITIES", "Activities", "活动组", "UInkActivitiesGroupPanel", "existing_preview", "UI_HUD", "UI-P3", "base", "ActivityVm", "切换日常与限时"),
        ("UI_DAILY", "DailyActivities", "日常活动", "UInkDailyActivitiesPanel", "existing_preview", "UI_ACTIVITIES", "UI-P3", "base", "ActivityVm", "查看真实目标"),
        ("UI_LIMITED", "TimedActivities", "限时活动", "UInkTimedActivitiesPanel", "existing_preview", "UI_ACTIVITIES", "UI-P3", "base", "ActivityVm", "查看已定时间活动"),
        ("UI_ACTIVITY_DETAIL", "DailyActivityDetail", "日常详情", "UInkDailyActivityDetailPanel", "existing_preview", "UI_DAILY", "UI-P3", "base", "ActivityVm", "参与允许活动"),
        ("UI_TIMED_DETAIL", "TimedActivityDetail", "限时详情", "UInkTimedActivityDetailPanel", "existing_preview", "UI_LIMITED", "UI-P3", "base", "ActivityVm", "查看期限及参与"),
    ]:
        screen(sid, route, title, current, native, "", parent, phase, gate, vm, action)
    screen("UI_LAUNCH", "LaunchEvent", "青云初启开服活动", "new_planned", "UInkLaunchEventPanel", BP + "WBP_InkLaunchEvent.uasset", "UI_ACTIVITIES", "UI-P1", "launch_activity", "LaunchVm|RewardVm", "查看任务共同目标与确认展示")
    screen("UI_LAUNCH_RANK", "LaunchRank", "开服玩法贡献榜", "new_planned", "UInkLaunchRankPanel", BP + "WBP_InkLaunchRank.uasset", "UI_LAUNCH", "UI-P1", "launch_activity", "LaunchVm", "查看实际贡献与荣誉名次")
    save_csv("ui-screens.csv", screens)

    files = []

    def file(path, action, phase, purpose, native="", screen_ids=""):
        files.append(dict(path=path, action=action, phase=phase, native_class=native, screen_ids=screen_ids, purpose=purpose, existed_at_audit=str((PROJECT / path).exists()).lower()))

    groups = {
        "InkUiScreenRegistry": ("UI-P0", "UInkUiScreenRegistry|UInkUiThemeData", "路由和主题DataAsset及现有路由兼容"),
        "InkUiViewModels": ("UI-P0", "UInkUiViewModelSubsystem", "唯一权威快照与请求状态映射"),
        "InkUiCommonWidgets": ("UI-P0", "UInkUiRequestStateWidget及共用组件", "共用控件不持有交易业务"),
        "InkUiRacePanels": ("UI-P2", "UInkRaceLobbyPanel|UInkRaceLivePanel|UInkRaceSettlementPanel|UInkResourceLeasePanel", "竞采匹配现场结算与租约"),
        "InkUiSwordPanels": ("UI-P1", "UInkSwordSpiritPanel|UInkGiftRulesPanel", "个人剑灵与礼物版本说明"),
        "InkUiMethodPanel": ("UI-P2", "UInkMethodsPanel", "主辅功法与准备期配置"),
        "InkUiEconomyPanels": ("UI-P3", "UInkShopPanel|UInkMarketPanel|UInkTradeOrdersPanel", "原型经济且发布候选关闭"),
        "InkUiRewardPanels": ("UI-P1", "UInkEventSettlementPanel|UInkOfflineRewardsPanel", "分归属奖励收据及离线开关"),
        "InkUiLaunchPanels": ("UI-P1", "UInkLaunchEventPanel|UInkLaunchRankPanel", "开服任务共同目标与荣誉榜"),
    }
    for stem, (phase, native, purpose) in groups.items():
        for suffix in (".h", ".cpp"):
            file(CP + "UI/" + stem + suffix, "add_planned", phase, purpose, native)
    for name, purpose in [
        ("InkFrontendWidget.h", "新增注册表与视图缓存引用"),
        ("InkFrontendWidget.cpp", "沿用主菜单与HUD接入状态"),
        ("InkFrontendExtensions.cpp", "新路由及返回栈和能力校验"),
        ("InkFeaturePanel.cpp", "优先注册表且保留旧路由"),
        ("InkWulingPanel.cpp", "已有武灵面板增量接线与子页"),
        ("InkFeaturePanels.cpp", "世界与五资源页面接线"),
        ("InkRewardsPanels.cpp", "签到月签与活动去占位"),
        ("InkInventoryWidget.cpp", "实例来源绑定期限与权限提示"),
        ("InkInventoryModel.h", "展示模型增加权威实例属性并保留200格"),
        ("InkInventoryModel.cpp", "不可出售及栈合并约束"),
        ("InkFrontendTypes.h", "显示数据字段与业务ID分离"),
    ]:
        file(CP + name, "modify_planned", "UI-P0_to_P3", purpose)
    for s in screens:
        if s["widget_asset"]:
            file(s["widget_asset"], "reuse" if s["widget_asset"].startswith(legacy) else "add_planned", s["phase"], s["title"], s["native_class"], s["screen_id"])
    for name in ("RequestState", "ConfirmModal", "MemberRow", "RewardItem", "ItemTooltip", "ContributionRow", "LeaseBadge", "GiftEffectCard", "Toast"):
        file("Content/UI/Ink/Wuling/Components/WBP_Ink" + name + ".uasset", "add_planned", "UI-P0", "共用" + name, "UInkUi" + name + "Widget")
    file("Content/UI/Ink/Wuling/Data/DA_InkWulingScreens.uasset", "add_planned", "UI-P0", "Route与WidgetClass唯一映射", "UInkUiScreenRegistry")
    file("Content/UI/Ink/Wuling/Data/DA_InkWulingTheme.uasset", "add_planned", "UI-P0", "引用现有皮肤与文字样式", "UInkUiThemeData")

    art = []
    for stem, w, h, cols, rows, units in [
        ("ItemIcons", 2048, 2048, 4, 4, "破境石|精炼核心|灵草|玄铁|残页|回复丹|护阵符|研习简|凝元精华|灵石|公共资材|个人贡献|武灵|开采权|绑定锁|提醒"),
        ("ResourceIcons", 1536, 1024, 3, 2, "灵泉|灵石矿|灵草园|玄铁脉|残卷遗址|共修"),
        ("MethodIcons", 2048, 1024, 4, 2, "疾锋|星罗|玄罡|回响|合契|燃脉|百蚀|空态"),
        ("Emblems", 1536, 1024, 3, 2, "青云|星河|玄铁|丹霞|松鹤|问道"),
        ("LaunchTitleEmblem", 512, 512, 1, 1, "飞雁徽记"),
    ]:
        source = "SourceAssets/UI/Wuling/Generated/Wuling" + stem + ".png"
        target = "Content/UI/Ink/Wuling/Textures/T_Wuling" + stem + ".uasset"
        art.append(dict(art_id="ART_" + stem.upper(), source_path=source, texture_path=target, width=w, height=h, columns=cols, rows=rows, alpha="true_rgba", safe_inset=32, cell_order="row_major_top_left", units=units, status="planned_generate_only_if_missing", style="water_ink_small_gold_no_baked_text"))
        file(source, "add_if_not_reusable", "UI-P1_to_P3", "图集源图")
        file(target, "add_if_not_reusable", "UI-P1_to_P3", "导入的UI图集")
    save_csv("ui-art-requests.csv", art)
    for path, purpose in [
        ("SourceAssets/UI/Wuling/atlas-regions.json", "实际像素切片与UV"),
        ("SourceAssets/UI/Wuling/art-manifest.md", "来源复用生成与导入记录"),
        ("Tools/InkFrontend/inspect_wuling_ui_assets.py", "只读资产引用审计"),
        ("Tools/InkFrontend/verify_wuling_ui_pie.py", "真实渲染与交互验证"),
        ("Tools/InkFrontend/wuling-ui-acceptance.json", "UI验收场景配置"),
    ]:
        file(path, "add_planned", "UI-P0_to_P3", purpose)
    save_csv("ui-files.csv", files)

    acceptance = []
    for s in screens:
        for case in ("Ready", "Empty", "Loading", "Pending", "Disconnected", "Error", "FeatureDisabled"):
            acceptance.append(dict(case_id=s["screen_id"] + "_" + case, screen_id=s["screen_id"], case=case, resolutions="540x960|720x1280|1280x720", expected="按13状态规范处理且可返回", scope="planned_runtime_verification", current_result="not_run"))
    for sid, case, expected in [
        ("UI_INVENTORY", "Capacity", "200总槽默认20解锁每页40保持物理索引"),
        ("UI_INVENTORY", "Binding", "绑定物禁售且不可合并不同期限栈"),
        ("UI_WULING", "Names", "12字名称三种人数与20行分页"),
        ("UI_MONTH", "Milestones", "累计3_7_14_21而非旧节点"),
        ("UI_RACE_RESULT", "Tie", "并列同档且领取不重开租期"),
        ("UI_LEASE", "Expired", "关播仍到期且不客户端发物"),
        ("UI_SHOP", "DoubleSubmit", "唯一请求与服务端收据"),
        ("UI_ORDERS", "PendingReconcile", "关页重回不重复入账"),
        ("UI_GIFT_RULES", "Legacy", "旧权益与候选描述分开"),
        ("UI_ROUND_RESULT", "DoubleClaim", "只确认展示不重复发奖"),
        ("UI_HUD", "ViewerControl", "不把视频按钮当作观众可点控件"),
    ]:
        acceptance.append(dict(case_id=sid + "_" + case, screen_id=sid, case=case, resolutions="540x960|720x1280|1280x720", expected=expected, scope="planned_runtime_verification", current_result="not_run"))
    save_csv("ui-acceptance.csv", acceptance)

    inventory = []
    for p in sorted((PROJECT / "Content/UI/Ink").rglob("*.uasset")):
        prefix = p.stem.split("_", 1)[0]
        inventory.append(dict(path=p.relative_to(PROJECT).as_posix(), object_path="/Game/" + p.relative_to(PROJECT / "Content").with_suffix("").as_posix(), kind=prefix, bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest(), status="file_exists_not_runtime_loaded"))
    save_csv("ui-existing-assets.csv", inventory)
    references = []
    for p in sorted(Path(r"C:\Users\zxxuh\Desktop\UI资源").rglob("*.png")):
        with Image.open(p) as image:
            width, height = image.size
        references.append(dict(path=str(p), bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest(), width=width, height=height, visual_review="inspected_this_turn"))
    (ROOT / "ui-audit/reference-images.json").write_text(json.dumps(references, ensure_ascii=False, indent=2), encoding="utf-8")
    source_files = []
    for folder in (PROJECT / CP, PROJECT / "Source/BoundlessImmortality/Wuling"):
        for p in sorted(folder.rglob("*")):
            if p.is_file() and p.suffix in (".h", ".cpp"):
                source_files.append(dict(path=p.relative_to(PROJECT).as_posix(), sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    (ROOT / "ui-audit/source-inspection.json").write_text(json.dumps(source_files, ensure_ascii=False, indent=2), encoding="utf-8")

    checks = []

    def check(name, condition):
        checks.append(dict(name=name, passed=bool(condition)))

    check("unique_screen_ids", len(screens) == len({s["screen_id"] for s in screens}))
    check("unique_routes", len(screens) == len({s["route"] for s in screens}))
    check("unique_file_paths", len(files) == len({f["path"] for f in files}))
    check("four_atlases_plus_title_emblem", len(art) == 5 and sum(a["columns"] * a["rows"] > 1 for a in art) == 4)
    check("seven_existing_wbp", sum(r["kind"] == "WBP" for r in inventory) == 7)
    baseline_textures = json.loads((ROOT / "ui-audit/texture-inspection.json").read_text(encoding="utf-8"))
    check("baseline_texture_files_preserved", len(baseline_textures) == 18 and all(Path(r["file"]).is_file() for r in baseline_textures))
    check("eleven_reference_images", len(references) == 11)
    check("fifteen_new_screen_wbp", sum(f["action"] == "add_planned" and f["path"].startswith(BP) for f in files) == 15)
    check("nine_new_component_wbp", sum("/Components/WBP_" in f["path"] for f in files) == 9)
    ids = {s["screen_id"] for s in screens}
    file_paths = {f["path"] for f in files}
    gates = {"base", "economic_inventory", "cross_host_race", "cross_session_numeric", "offline_cultivation", "npc_shop", "player_trade", "launch_activity"}
    for s in screens:
        check(s["screen_id"] + "_parent", not s["parent"] or s["parent"] in ids)
        check(s["screen_id"] + "_asset_manifest", not s["widget_asset"] or s["widget_asset"] in file_paths)
        check(s["screen_id"] + "_gate", s["feature_gate"] in gates)
    for f in files:
        if f["action"] in ("reuse", "modify_planned"):
            check("current_file_" + f["path"], f["existed_at_audit"] == "true")
        elif f["action"] == "add_planned":
            check("planned_not_existing_" + f["path"], f["existed_at_audit"] == "false")
    for a in art:
        check(a["art_id"] + "_geometry", a["width"] % a["columns"] == 0 and a["height"] % a["rows"] == 0 and len(a["units"].split("|")) == a["columns"] * a["rows"])
    check("runtime_tests_not_claimed", all(a["current_result"] == "not_run" for a in acceptance))
    report = dict(document_version="1.3", passed=all(c["passed"] for c in checks), check_count=len(checks), failed=[c for c in checks if not c["passed"]], screen_count=len(screens), planned_file_rows=len(files), planned_art_assets=len(art), planned_art_atlases=4, runtime_acceptance_cases=len(acceptance), existing_asset_files=len(inventory), source_files_inspected=len(source_files), reference_images_reviewed=len(references), scope="UI策划清单完整性与当前文件存在性；未执行UE或平台UI测试", checks=checks)
    (ROOT / "ui-planning-validation.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in report.items() if k != "checks"}, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
