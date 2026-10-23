# 水墨 UI 字体资源包

本目录保存实际字体二进制和各自许可证，已从官方仓库下载；不需要从网页找下载按钮，也没有安装进Windows。文件来源、固定commit、下载时间、大小和SHA256见 [font-manifest.json](font-manifest.json)。

| 角色 | 文件 | 建议 |
|---|---|---|
| 书法标题 | [MaShanZheng-Regular.ttf](MaShanZheng/MaShanZheng-Regular.ttf) | 页标题/短主题词；不用于价格、长说明和密集属性 |
| 所有正文与数字 | [LXGWWenKai-Regular.ttf](LXGWWenKai/LXGWWenKai-Regular.ttf) | 正文28、按钮28–30、短标签24–26；血量、价格、数量、日期、百分比、倒计时与昵称均使用文楷 |
| 仅缺字回退 | [NotoSansCJKsc-Regular.otf](NotoSansSC/NotoSansCJKsc-Regular.otf) | 只补文楷没有的字符，不作为任何常规文本或数字的默认字体 |

字体不是从概念图提取的原字库；它们是已获得开源文件、风格匹配且适合分工的组合。按用户最新要求，所有文本包括数字使用修仙风格字体：标题马善政，其余文楷。Noto只为缺字准备静态Regular OTF。用字体试排检查实际字面，不能仅凭同样字号就认为高度一致。

`NotoSansSC[wght].ttf` 与其 `OFL.txt` 保留作上轮下载追溯，在清单中标记为 `legacy_variable_reference`；不要把它和静态Regular同时导入同一默认角色。静态OTF经本轮官方Git blob校验，许可证为同一固定commit的 `Sans/LICENSE`。清单中的下载时间未知项保留null，以核验时间另列，避免补造时间。

## UE 导入建议（尚未执行）

建议新增目录 `/Game/UI/Ink/Typography/`：

| 计划资产 | 来源/配置 |
|---|---|
| `FF_InkDisplay` | 马善政 Font Face |
| `FF_InkBody` | 霞鹜文楷 Font Face |
| `FF_InkData` | 复用文楷Font Face；数字不切换到黑体 |
| `FF_InkFallback` | Noto Sans CJK SC Regular Font Face，仅缺字 |
| `F_InkDisplay` | 新建Runtime Composite Font，马善政为默认，先文楷再Noto补缺字 |
| `F_InkBody` | 新建Runtime Composite Font，文楷为默认，Noto为回退 |
| `F_InkData` | 新建Runtime Composite Font，文楷为默认，Noto仅补缺字 |

按引擎实际导入选项创建Font Face/Font并测试，不假定拖入文件就自动得到同名Composite Font。昵称/扩展汉字不保证任何字库全覆盖；缺字时走回退并使用明确的替代策略，记录样例。不得悄悄改现有 `/Game/LiveWeapons/Fonts/F_LiveNames`，避免影响场景姓名与其他系统。

Package/Cook时确认Font Face均有实际引用且进入产物；正文选择Runtime缓存，长列表预热常用字符并观察首次字形生成卡顿。若以后子集化，保留完整源字体和许可证，依据真实本地化/昵称范围决定；本次下载的是完整文件，未子集化、未改名、未合并。

## 许可证随包

每款附带上游OFL原文：`LXGWWenKai/OFL.txt`、`MaShanZheng/OFL.txt`、`NotoSansSC/LICENSE`。分发时一起保留版权与许可文件；字体不可单独作为商品售卖，修改/重命名等按各自OFL条款处理。本说明不是额外的版权承诺，具体以随附原文为准。

来源：[文楷](https://github.com/lxgw/LxgwWenKai)、[马善政项目](https://github.com/googlefonts/mashanzheng)、[Noto CJK](https://github.com/notofonts/noto-cjk)。技术依据：[Epic Font Asset and Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/font-asset-and-editor-in-unreal-engine)。
