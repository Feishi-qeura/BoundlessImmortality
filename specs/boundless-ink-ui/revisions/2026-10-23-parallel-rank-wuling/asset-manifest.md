# 本组资产清单

所有图均为候选，`approved: false`。实际测量来源：`measured-assets.json`；RGB PNG；未做尺寸缩放。

| asset_id | 页面 | 文件 | 实际像素 | SHA256 | 提示词 |
|---|---|---|---|---|---|
| rw-leaderboard-damage-v1 | leaderboard | assets/pages/rw-leaderboard-damage-candidate-v1.png | 941×1672 RGB | D4CD72D387408A6857E7E70FEEC938B81880E4EA48D2E1CE9CF69EE003D5511B | prompts/leaderboard-states.md#伤害榜 |
| rw-leaderboard-cultivation-v1 | leaderboard | assets/pages/rw-leaderboard-cultivation-candidate-v1.png | 940×1672 RGB | FA34F445CF829969982875E76740B849716DD06E76700F53623A38D23E6D157F | prompts/leaderboard-states.md#修炼速度榜 |
| rw-leaderboard-spirit-v1 | leaderboard | assets/pages/rw-leaderboard-spirit-candidate-v1.png | 941×1672 RGB | 627E7E98DFC077DC535313B494F9FD85BD9DCAE6B2D37CAA9649A9D790974A62 | prompts/leaderboard-states.md#主播剑灵榜 |
| rw-wuling-home-v1 | wuling | assets/pages/rw-wuling-home-candidate-v1.png | 941×1672 RGB | 89EA9C91683DC96B85C13EB90CFFBAC729ED7EA6973132F4800417EF3B3FC7A7 | prompts/wuling-home.md |

## v2 图像记录

| asset_id | 文件 | 状态 | 实际尺寸/模式 | SHA256 |
|---|---|---|---|---|
| rw-leaderboard-damage-v2 | assets/pages/rw-leaderboard-damage-candidate-v2.png | candidate; approved=false | 941×1672 RGB | 4984754FB5A5ABE8562F76B5AD72EE0320243160F1CE31C9373FD950743BDE8D |
| rw-leaderboard-cultivation-v2 | assets/pages/rw-leaderboard-cultivation-candidate-v2.png | candidate; approved=false | 941×1672 RGB | 355A1040FE205E8DD5C1D4DFFB7FB2CA165FF35C227EAC6A1880F18F683C7456 |
| rw-leaderboard-spirit-v2 | assets/pages/rw-leaderboard-spirit-candidate-v2.png | candidate; approved=false | 940×1672 RGB | A0EB106064FBA54D8D51543671E446697E4DC6714A5BB8F268403B415C9B79FC |
| rw-wuling-home-v2 | assets/pages/rw-wuling-home-candidate-v2.png | candidate; approved=false | 940×1672 RGB | 2F9D45EDFDE5014CBE6A2F98A6FC31D05B3A5EFF204E74E3B3A686490A443176 |

v1 文件保留且未覆盖。v2 经人工视觉检查：榜单三张采用相同的空表格布局，剑灵榜仍显示主播剑灵对象字段；武灵页为加载状态，管理入口锁定并标示权限字段控制。生成器仍可能对占位词做视觉化解释，动态文本应由运行 UI 实际排版。
