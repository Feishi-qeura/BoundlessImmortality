# 每日签到与月签到

## 页面定位
展示每日签运和奖励领取状态，同时承接月度日历累计奖励。
## 视觉约束
水墨签到卷轴、月历格纸面、奖励用轻量玉色/旧金等级，不做抽奖机视觉。
## 布局与组件
上方每日签运卡突出大吉/吉/平/凶结果、短说明和今日奖励；下方七日累计轨道。月签子页显示月历与累计进度、可领取状态。底部固定签到 CTA。
## 状态
已签、待签、处理中、服务错误；月签可领/已领/补签关闭。
## 图像生成提示词
Portrait 9:16 Chinese xianxia daily sign-in UI, calm paper-and-ink design. Top daily fortune card with four clearly differentiated fortune seal placeholders (great auspicious, auspicious, neutral, inauspicious), a short explanation region and reward icon placeholders; below, a seven-day cumulative reward track with one highlighted claimable state; fixed bottom sign-in action. Add a secondary month-calendar preview tile. Ivory paper, ink green, restrained jade and antique gold, subtle red vermilion stamp only for fortune/emphasis, legible blank text regions, crisp spacious mobile layout, no casino/gacha wheel, no re-roll, no pay-to-change fortune, no fake text or logo.
## 反向限制
不随机生成签运，不暗示补签或改运可付费，不按设备当前日期显示已领结果。
## 验收记录
待审；签运、奖励与 claim state 均必须由 ActivityVm 返回。
