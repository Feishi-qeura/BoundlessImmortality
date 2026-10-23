# 候选视觉验证记录

- 日期：2026-10-23
- 盘点范围：本组`assets/`目录中的PNG文件，以实际文件为准。
- 结果：4/4存在且可解码；均为941×1672、PNG、Format24bppRgb（RGB）。实际宽高比约0.5628，目标9:16约0.5625。
- 文件和SHA256：
  - `world-map-v2-candidate.png` — `7912af89ce9dedcf9198394f140514166deab5ad72139b1daeb504b7402a1390`
  - `resources-map-v1-candidate.png` — `517a6ff150170b4b4790bb1697530064bde64945d3de285c2669c181d8999eb5`
  - `daily-sign-in-v1-candidate.png` — `e8cc785139514135cf6d2cbb73a2b46796037558d425981231f35c38a26e0505`
  - `monthly-sign-in-v1-candidate.png` — `37b9041200bca28618886a8ca9f9fc7ec94a103a0d585e7d710c5ed5defe6f7c`
- 四项均在`asset-manifest-candidate.yaml`登记为`status: candidate`、`approved: false`，生成提示词路径均存在。
- 检查边界：确认文件存在、PNG可读、实际尺寸/像素模式/哈希；未据此声称中文、视觉布局或业务规则已通过人工审核。
- 未运行UE/PIE、设备或平台验证；未修改共享契约或UE代码。
