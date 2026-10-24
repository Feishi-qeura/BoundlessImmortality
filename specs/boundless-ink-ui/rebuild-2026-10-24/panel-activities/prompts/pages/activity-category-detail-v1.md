# Activity Category Detail

## 页面定位

每日/限时/常驻活动的分类内列表与当前选中活动详情；不自动触发领取或进度。

## 视觉约束

- 竖屏优先 720×1280、9:16、24逻辑安全边距；横屏改成列表与详情并列。
- 复用 slice-01 的墨黑、青玉、暖白、低饱和旧金、清晰按钮状态。内容密度高于活动中心，背景图退到极弱边缘。
- 真实活动文字与数值运行时排版；预览图不作为界面底图。

## 布局与组件

- 顶部类别标题与返回意图，随后是每日/限时/常驻切换轨。
- 活动列表采用短行而非海报卡；状态、摘要、周期分开排版。
- 当前选中详情展示标题、说明、参与条件、真实服务日/期限、进度与奖励预算（仅提供时）。
- ACT_SIGN仅在服务明确提供“未签到”并有服务日时显示可操作按钮；其他参与按钮展示接口未开放原因。

## 状态

加载、离线、空列表、选中条目缺失、资料缺字段、服务禁用、ACT_SIGN可签/已签/状态未知。数据冲突以适配器视图为准。

## 图像生成提示词

Complete portrait mobile Chinese cultivation MMO category-detail screen, 9:16 at 720×1280. Use a refined compact ink-jade interface: clear top category title, three short selected-state tabs, a vertical list of compact activity rows with state chips, then a strong selected-detail surface with readable text slots and one primary action zone. Use dark pine-ink header, muted celadon selection, warm ivory content surface, thin antique-gold separators, restrained mountain mist only in edge accents. Show a clear hierarchy between list and detail and enough spacing for 88px touch targets. Leave variable strings, progress, dates and rewards blank for runtime text; avoid oversized poster art and empty decorative space. Visual-only preview, not an importable page texture.

## 反向限制

No fake progress, deadline, reward, fortune, count, paid action, generic claim button, glitched Chinese, duplicated heavy borders, unread red dot without data, or copied old screenshot.

## 验收记录

Reuses the existing batch-2 detail concept only as a layout sketch; the new native widget is independently composed. Candidate, `approved:false`.
