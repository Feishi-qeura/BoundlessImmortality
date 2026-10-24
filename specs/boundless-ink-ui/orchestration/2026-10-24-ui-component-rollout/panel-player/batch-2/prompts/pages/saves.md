# 存档页 Saves

## 页面定位
首页入口的本地三槽存档页。真实槽数据仅取自 `UInkSaveSubsystem::GetSlots()`；空位/无效档/有效档由 `FInkSaveSlotInfo` 给出。

## 视觉约束
720×1280竖屏、1280×720横屏。承接已批准水墨风格契约：暖宣纸、墨绿正文、少量旧金边、朱砂只用于删除警示。无场景角色，无造字，无业务数据。图片只作版式概念，不作运行底板。

## 布局与组件
页头固定标题和返回；竖屏中部滚动显示三个槽卡，横屏左侧槽列表、右侧当前槽详情；底部固定主操作区。每槽预留标题、存档时间、地图名、游玩时长和操作行，值全部留空给 TextBlock。无效存档区域预留真实错误原因。覆盖和删除继续使用宿主已有确认覆盖层。

## 状态
loading、ready、pending、error、locked/unavailable；pending/error文案必须由实际宿主提供。槽位空/坏档/版本不兼容按真实 `bExists`、`bValid`、`ValidationError` 渲染。手动存档为同步 API，未观测到实际处理中时不显示 pending。

## 图像生成提示词
Create one portrait 9:16 mobile game saves page layout concept for an original Chinese cultivation RPG, reference size 720x1280, no device frame. Inherit a restrained premium ink-and-paper UI vocabulary: warm ivory paper, deep ink-green structure, pale jade accents and sparse antique-gold hairline borders, soft top-left light, ample readable spacing. Show a fixed compact header with a blank title area and a clear back affordance. Below it show exactly three equal neutral save-slot cards as empty wireframe cards with no selected-vs-existing distinction: each card has a blank upper label area, two short empty information lines, and a bottom row of three blank action-control silhouettes. Keep all data regions visibly blank; do not imply any slot is actually empty, valid, saved, or playable. Put a quiet fixed footer area with one wide blank primary-action silhouette and a smaller blank supporting status strip. Keep body independently scrollable and preserve 24-unit safe margins and 88-unit minimum thumb targets. No characters, scenery, logos, letters, numbers, dates, map names, play time, status icons, checkmarks, warnings, prices, rewards, or fake UI text. The image is only a layout concept; all runtime copy/data will be rendered by UE TextBlocks.

## 反向限制
No real game screenshots, no copied store art, no text/glyphs/numbers, no fake saved slot content, no purported success/failure badges, no additional save types, no cloud save, no extra buttons, no invented business actions, no full-page opaque background, no clipped or distorted controls, no ornament-heavy frames.

## 验收记录
Candidate prompt only. Output remains approved:false; visual pending user. Do not import this page concept as a runtime background. Actual state/action rendering comes from staged C++ and existing subsystem APIs.
