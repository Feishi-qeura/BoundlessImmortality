# Activities Hub

## 页面定位

全新活动中心入口。玩家能立即看见活动服务状态、开服主活动入口和每日/限时/常驻三类内容；进入下一层后查看具体数据。

## 视觉约束

- UE 5.8 native UMG，手机竖屏优先，9:16，逻辑画布 720×1280；安全区24逻辑单位。内容中段可滚动，主入口至少88×88触控目标。
- 以项目当前 ink/jade/muted-gold/paper 颜色与中文字体字号作临时令牌；采用清楚的暗墨标题栏、紧凑活动卡列、明确选中条，避免旧版大纸框和层层空白面板。
- 页面概念供布局评审，所有动态文字、日期、状态和数字由运行时 TextBlock 绘制。

## 布局与组件

- 顶部：返回、单行「活动」标题、服务器连接状态与服务日（缺失时不造值）。
- 首屏焦点：开服活动横向主入口，显示服务状态和一条实际说明，带独立查看按钮；若无活动则显示服务资料待同步。
- 次行：每日/限时/常驻三态切换器；选中态用明亮青玉底和金线标记，不以红点代替服务端未读状态。
- 主体：两到四条紧凑活动行，左侧语义图标槽，中央标题/摘要，右侧状态标记和箭头。列表可纵向滚动。
- 空、离线、加载状态占据同一内容位；底部保留返回/关闭手势区域，不重复全局导航。

## 状态

加载、离线、无条目、可查看、活动关闭、部分字段待同步。类别计数只有模型声明来源字段时显示。活动入口不显示未验证奖励或倒计时。

## 图像生成提示词

Design one complete portrait mobile game UI concept for a Chinese cultivation MMO Activities Hub, 9:16 ratio, 720×1280 logical composition. Build a fresh compact editorial layout: narrow deep-ink masthead, one wide featured event strip, a three-way category selector, then a dense vertical feed of distinct activity rows with a small jade icon well, title/summary space, right-side state chip, and arrow. Use restrained ink-black, celadon jade, muted antique gold, warm ivory paper, subtle mountain mist only at the edges. Clear Chinese text hierarchy, generous touch spacing, readable positive/selected/disabled states, clean alignment, no giant empty paper cards. Make every activity row visually separable but keep the whole list compact. Use short Chinese labels exactly: “活动”, “开服”, “每日”, “限时”, “常驻”, “查看”. Keep dates, numbers and reward values as blank dynamic-text fields. No full-screen illustration that competes with controls.

## 反向限制

No garbled text, fabricated rewards/prices/progress/countdowns, red-dot decoration without unread data, multiple oversized posters, nested heavy frames, copyrighted characters, logos, old screenshot tracing, baked dynamic values, clipping, overlapping controls, or page-as-runtime-texture delivery.

## 验收记录

Candidate concept only; `approved:false`. Runtime page uses independent UUserWidget controls and a typed view-model/action delegate.
