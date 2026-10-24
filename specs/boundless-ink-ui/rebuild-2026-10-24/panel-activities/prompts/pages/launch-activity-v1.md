# Launch Activity / Tasks

## 页面定位

展示青云开服活动的权威状态、个人任务/组织目标、开服入口和特惠可用性。贡献榜为单独页面。

## 视觉约束

- 9:16、720×1280逻辑布局，竖屏优先；安全边距24，长任务区独立滚动，底部动作区不覆盖内容。
- 采用新的分区式活动仪表盘，不复用旧 launch WidgetTree；墨色页头、暖白任务列表、青玉状态和少量旧金主操作。
- 日期必须是服务器提供的起止值；数值奖励按权限显示；特惠无SKU/价格/购买接口时清楚禁用。

## 布局与组件

- 顶部身份区：标题、状态chip、真实活动起止时间。
- 横向分段标签：总览、个人任务、组织目标、荣誉摘要。
- 主内容：任务使用高密度行/卡显示条件、进度、确认态；服务自动结算，不提供领取按钮。
- 固定底部操作区：仅当适配器提供可开启授权时显示开启入口；点击后出现二次确认；活动已激活则显示进入历练。特惠卡作为禁用信息区，原因常显。

## 状态

未同步、等待服务、活动未开启、活动进行中、已结束/收尾、任务为空、进度未知、自动结算确认、特惠资料/接口缺失。

## 图像生成提示词

Design one full portrait game UI concept for a Chinese cultivation MMO launch-event page, 9:16 at 720×1280. Fresh structured mobile dashboard: compact deep-ink event masthead with jade state chip and two blank date slots; a concise personal contribution strip; four narrow selected-state tabs; a vertically scrollable list of clearly distinct task rows with icon wells, progress rail placeholders and small server-confirmed badges; a lower offer panel in visibly disabled gray-jade with an explanatory blank text area; one persistent primary action slot. Keep text legible with short exact labels “青云初启”, “总览”, “个人任务”, “组织目标”, “荣誉”, “待同步”, “暂不可用”. Use warm ivory, pine ink, celadon and restrained antique gold. Dynamic numbers/rewards remain empty placeholders. This is a composition mockup, not runtime art.

## 反向限制

No invented item/reward/prices, SKU, purchase CTA, rank or progress; no client claim button; no page-wide background PNG, excessive frames, overcrowded Chinese, distorted buttons or old page screenshot tracing.

## 验收记录

Generated as a composition concept for the new native UMG page; not a runtime texture. Candidate, `approved:false`.
