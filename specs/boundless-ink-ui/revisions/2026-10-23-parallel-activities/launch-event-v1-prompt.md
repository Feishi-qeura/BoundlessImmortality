# 青云初启开服活动 `launch` 页面提示词

状态：可复现候选提示词；未批准。

## 页面定位

展示开服活动真实剩余时间、免费玩法贡献、每日任务、里程碑、贡献榜入口及可选获准商品。9:16，720×1280，边缘安全区24。

## 视觉约束

批准水墨纸风：#F3EBDD宣纸、#202823墨绿、#3D5861青灰、#B88B45旧金少量、#3A8D83玉色；柔和左上光、薄纸纹、墨山边缘、细描边、8圆角；文字留槽供客户端准确排字。

## 布局与组件

页头青云初启和活动状态；首屏紧凑倒计时与本人免费贡献进度并列但不混淆；四个内栏总览/每日任务/武灵目标/规则；中段独立滚动，任务卡显示名称、当前/目标、可领/已领状态和奖励空位；1/3/6/12里程碑沿纵向小节点轴排列；贡献榜预览独立卡片，明确“玩法贡献”；底部特惠 SKU 区与免费奖励视觉分隔，SKU无数据时整个区域不出现。页底固定单个下一任务 CTA。价格、折扣、库存全部留空并须后排字，不生成占位数字。

## 状态

未开放、活动中、结束、每日任务为空、无商品 SKU、结算中、断线旧快照、profile禁用。结束态保留规则和结果但移除领奖/购买 CTA。

## 图像生成提示词

Create a portrait 9:16, 720x1280 logical mobile game UI screen for a Chinese xianxia MMO event titled 青云初启. Use a fixed compact header with back affordance, authoritative event state, and a visually clear server countdown field intentionally left blank for runtime typography. Under it show two separate summary blocks: free gameplay contribution progress and the next daily objective; a compact contribution leaderboard preview labeled 玩法贡献; four local tabs 总览 / 每日任务 / 武灵目标 / 规则. Main independently scrolling content contains readable daily mission rows with blank server text lines, progress bars and reward placeholders, then a vertical 1/3/6/12 round milestone track with locked/earned visual states. A distinct lower special-offer region can be omitted entirely if no authorized SKU exists; if present, show only blank product title/cost fields and no numeric price. Keep paid offers clearly separated from free tasks and rewards. Fixed bottom action area has a single next-objective affordance. Refined restrained ink-on-xuan-paper visual matching premium cultivation game UI, ivory #F3EBDD, ink green #202823, blue-gray #3D5861, jade #3A8D83, antique gold #B88B45 used sparingly, soft upper-left light, subtle paper grain, distant ink mountain wash, fine strokes, 8 px card corners, Chinese typesetting zones left blank, high legibility, spacious 88 px touch targets, 24-unit safe area, no full-page scaling.

## 反向限制

Never fabricate task names, rewards, countdowns, contribution scores, prices, discounts, stock, purchase rights, VIP benefits, or payment success. No pay-to-win rhetoric, gambling, spending leaderboard, damage ranking, logos, illegible small text, fake Chinese, overflowing cards, excessive gold, CTA after expiration.

## 验收记录

候选未批准；时间/任务/商品全部绑定权威动态字段。候选图已落盘，具体实测信息见 asset-manifest-proposed.yaml。

### 最终生图提示词（v1）

Portrait 9:16 720x1280 Chinese xianxia game UI 青云初启 launch event. Header with back and state, a blank authoritative countdown, separate free contribution progress and next daily mission. Local tabs 总览 每日任务 武灵目标 规则. Vertical daily mission rows with blank text/progress and reward placeholders; 1/3/6/12 milestone track; contribution ranking preview. Paid offer area visually separate and only blank, no price or number, may be absent. Fixed bottom single next objective action. Ivory xuan paper, ink green, jade, restrained old gold, clear Chinese, 24 safe inset, scroll middle, refined water-ink. Never invent rewards, SKU prices, discounts, timer digits, purchase eligibility or fake text.
