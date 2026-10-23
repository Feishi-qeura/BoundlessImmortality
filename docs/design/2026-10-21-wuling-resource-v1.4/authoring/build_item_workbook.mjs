import fs from 'node:fs/promises';
import path from 'node:path';
import { createRequire } from 'node:module';
import { pathToFileURL, fileURLToPath } from 'node:url';
const dir=path.dirname(fileURLToPath(import.meta.url));
const root=path.dirname(dir);
// Sandbox disallows a node_modules junction. Resolve the supported package export
// from the read-only bundled runtime instead of creating or inspecting dependencies.
const require=createRequire('C:/Users/zxxuh/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/package.json');
const {Workbook,SpreadsheetFile}=await import(pathToFileURL(require.resolve('@oai/artifact-tool')).href);
const data=JSON.parse(await fs.readFile(path.join(dir,'workbook_data.json'),'utf8'));
const count=JSON.parse(await fs.readFile(path.join(root,'tables/items-v1.4/counts.json'),'utf8'));
const bands=JSON.parse(await fs.readFile(path.join(root,'tables/items-v1.4/item_system.json'),'utf8'));
const wb=Workbook.create();
const cover=wb.worksheets.add('配置总览');
const quality=wb.worksheets.add('品质计算');
const trib=wb.worksheets.add('天劫计算');
const font='Arial';
const ink='#243A42', gold='#AF8549', muted='#EFF2F2';
function style(sheet,range){
  sheet.showGridLines=false;
  sheet.getRange(range).format.font={name:font,size:10,color:'#24343B'};
  sheet.getRange(range).format.verticalAlignment='center';
  sheet.getRange(range).format.rowHeight=24;
}
function heading(s,title,subtitle,last='H'){
  s.getRange('A2').values=[[title]]; s.getRange('A2').format.font={name:font,size:15,bold:true,color:ink};
  s.getRange(`A3:${last}3`).format.borders={bottom:{style:'thin',color:gold}};
  s.getRange('A3').values=[[subtitle]];s.getRange('A3').format.font={italic:true,color:'#64747B',size:10};
}
function header(s,r){s.getRange(r).format={fill:ink,font:{name:font,size:10,color:'#FFFFFF',bold:true},rowHeight:30,horizontalAlignment:'center',verticalAlignment:'center',wrapText:true};}
function colname(n){let s='';for(n++;n;n=Math.floor((n-1)/26))s=String.fromCharCode(65+(n-1)%26)+s;return s;}
style(cover,'A1:H24');heading(cover,'武灵修仙物品配置 v1.4','十大突破材料与分表，数值为封闭原型设计，未写入游戏');
cover.tabColor=ink;cover.getRange('A1:H24').format.columnWidth=18;
cover.getRange('A5:D5').values=[['配置类别','记录数','主要用途','状态']];header(cover,'A5:D5');
const summary=[['突破材料种类',count.material_species,'每境界核心1，护道/精华/奇珍各5','已配置'],['突破材料品质条目',count.material_item_variants,'每种下品/中品/上品/极品','已配置'],['功法物品',count.method_items,'七部功法原典及定向研习卷','已配置'],['装备物品',count.equipment_items,'主播法衣/道冠/云履/灵佩','已配置'],['法宝',count.fabao_items,'主播同时装备1件','已配置'],['新道具',count.new_prop_items,'战斗辅助和演出，不买渡劫概率','已配置'],['武灵建设',count.building_level_rows,'8设施，每设施3级，公共外显','已配置'],['来源渠道',count.acquisition_sources,'具体区域/定向历练/炼制/公库','已配置'],['炼制配方',count.recipes,'确定性配方，不随机出品质','已配置'],['物品总表',count.master_items_including_legacy,'含24项既有兼容条目','设计数据']];
cover.getRange('A6:D15').values=summary;
cover.getRange('A6:D15').format.rowHeight=32;
cover.getRange('A5:A15').format.columnWidth=25;cover.getRange('B5:B15').format.columnWidth=12;
cover.getRange('C5:C15').format.columnWidth=48;cover.getRange('D5:D15').format.columnWidth=18;
cover.getRange('B6:B15').setNumberFormat('#,##0');
cover.getRange('A18').values=[['主播材料突破与观众剑灵自动突破分别处理。']];
cover.getRange('A19').values=[['天劫：基础50%，每名不同助力+1个百分点，最多50名。']];
cover.getRange('A20').values=[['失败损失最多1500修为，材料保留，不掉境界；连续失败5次后保底。']];
cover.getRange('A21').values=[['2.5～5倍保留为独立游戏参考，直播原型不逐次乘入。']];
cover.getRange('A22').values=[['正式发布的物品扩展与天劫默认关闭。']];
for(const d of data){
 const s=wb.worksheets.add(d.sheet), last=colname(d.headers.length-1), end=d.rows.length+5;
 style(s,`A1:${last}${end}`);heading(s,d.sheet+'配置','参数来源：'+d.file,last);
 s.getRange(`A5:${last}5`).values=[d.headers];s.getRange(`A6:${last}${end}`).values=d.rows;
 s.getRange(`A5:${last}${end}`).format.columnWidth=21;
 s.getRange(`A6:${last}${end}`).format.wrapText=true;
 s.getRange(`A6:${last}${end}`).format.rowHeight=40;
 for(let c=0;c<d.keys.length;c++){
   const k=d.keys[c],col=colname(c),r=s.getRange(`${col}5:${col}${end}`);
   if(/description|mechanic|condition|output_rule|benefit|use$|note$|rule$|mint_source|spend|exchange_ratio|stacking/.test(k))r.format.columnWidth=54;
   else if(/_id$|^item_id$|^output_item$|^material_[12]$/.test(k))r.format.columnWidth=31;
   else if(d.rows.some(row=>typeof row[c]==='number')){
     r.format.columnWidth=16;
     s.getRange(`${col}6:${col}${end}`).setNumberFormat(/value|magnitude|rate/.test(k)?'0.00':'0');
   }
 }
 s.tables.add(`A5:${last}${end}`,true,'ItemConfig_'+d.sheet.replace(/[^\x00-\x7F]/g,'')+data.indexOf(d));
 header(s,`A5:${last}5`);s.freezePanes.freezeRows(5);s.freezePanes.freezeColumns(2);
}
style(quality,'A1:K27');heading(quality,'突破品质计算','黄色为示例投入；确定性评分，不抽取品质','K');
quality.getRange('A1:K27').format.columnWidth=19;
quality.getRange('A5:F5').values=[['品质','核心分','护道权重','精华权重','奇珍分','合成输入数']];header(quality,'A5:F5');
quality.getRange('A6:F9').values=[['下品',10,3,6,4,2],['中品',18,6,14,9,2],['上品',26,10,22,15,2],['极品',35,15,30,20,2]];
quality.getRange('H5:K5').values=[['Q下限','突破品质','独立游戏参考倍率','品质预算系数']];header(quality,'H5:K5');
quality.getRange('H6:K11').values=[[0,'凡品',2.5,1],[40,'良品',3,1.02],[60,'上品',3.5,1.04],[75,'极品',4,1.06],[90,'天品',4.5,1.08],[98,'完美',5,1.10]];
quality.getRange('A14:D14').values=[['类别','投入权重总和','本阶段归一数量','贡献分']];header(quality,'A14:D14');
quality.getRange('A15:C18').values=[['核心',35,1],['护道',45,3],['精华',120,4],['奇珍',20,1]];
quality.getRange('D15:D18').formulas=[['=B15'],['=ROUNDDOWN(B16/C16,0)'],['=ROUNDDOWN(B17/C17,0)'],['=B18']];
quality.getRange('B15:C18').format.fill='#FFF0BE';
quality.getRange('A20:A23').values=[['品质总分'],['确定突破品质'],['本境界品质加成'],['十境界累计上限']];
quality.getRange('B20').formulas=[['=SUM(D15:D18)']];
quality.getRange('B21').formulas=[['=VLOOKUP(B20,H6:K11,2,TRUE)']];
quality.getRange('B22').formulas=[['=(VLOOKUP(B20,H6:K11,4,TRUE)-1)/10']];
quality.getRange('B23').values=[[bands.breakthrough.quality_total_bonus_cap]];
quality.getRange('B22:B23').setNumberFormat('0.0%');
quality.getRange('A25').values=[['每个大境界只记最高品质快照，十份加法封顶10%，不逐次相乘。']];
quality.getRange('A26').values=[['Q87固定为极品，独立参考4.0倍；直播本境界品质预算+0.6%。']];
quality.getRange('A27').values=[['护道权重按总量一次取整；天劫成功率在另一张表独立计算。']];
style(trib,'A1:H23');heading(trib,'天劫成功率与失败损失','从冲击元婴开始；黄色可修改，次数按已确认失败数');
trib.getRange('A1:H23').format.columnWidth=21;trib.getRange('A5:A13').format.columnWidth=28;
trib.getRange('A5:B9').values=[['基础成功率',.5],['有效不同助力人数',0],['此前连续失败次数',0],['尝试前未花费修为',5400],['失败扣修上限',1500]];
trib.getRange('D5:E8').values=[['每名助力增加',.01],['每次失败保底增加',.10],['保底增加上限',.50],['有效概率助力上限',50]];
trib.getRange('B5:B9').format.fill='#FFF0BE';trib.getRange('B5').setNumberFormat('0%');trib.getRange('E5:E7').setNumberFormat('0%');
trib.getRange('A11:A13').values=[['本次最终成功率'],['本次失败扣除修为'],['无助力最多尝试次数']];
trib.getRange('B11').formulas=[['=MIN(1,B5+MIN(B6,$E$8)*$E$5+MIN(B7*$E$6,$E$7))']];
trib.getRange('B12').formulas=[['=MIN(B8,B9)']];
trib.getRange('B13').formulas=[['=1+ROUNDUP((1-B5)/E6,0)']];trib.getRange('B11').setNumberFormat('0%');
trib.getRange('B6:B7').dataValidation={rule:{type:'whole',operator:'between',formula1:0,formula2:10000}};
trib.getRange('B8:B9').dataValidation={rule:{type:'whole',operator:'between',formula1:0,formula2:2147483647}};
trib.getRange('A16:D16').values=[['流程','持续秒数','观众可助力','材料状态']];header(trib,'A16:D16');
trib.getRange('A17:D21').values=[['准备与锁定',15,'尚未计数','预留未消耗'],['肉身劫',20,'是','预留未消耗'],['神魂劫',20,'是','预留未消耗'],['心魔劫',20,'是','预留未消耗'],['确认与揭示',8,'否','按唯一事务结算']];
trib.getRange('A23').values=[['失败保留材料且不掉境界；修为重新修满并过60秒可重试。']];
await wb.recalculate();
const formulaChecks=[];
function check(name,value){if(!value)throw new Error('Formula verification failed: '+name);formulaChecks.push(name);}
check('quality_100',quality.getRange('B20').values[0][0]===100);
check('perfect_label',quality.getRange('B21').values[0][0]==='完美');
check('base_50',trib.getRange('B11').values[0][0]===.5);
trib.getRange('B6').values=[[50]];await wb.recalculate();check('fifty_helpers_100',trib.getRange('B11').values[0][0]===1);
trib.getRange('B6').values=[[0]];trib.getRange('B7').values=[[5]];await wb.recalculate();check('pity_100',trib.getRange('B11').values[0][0]===1);
trib.getRange('B7').values=[[0]];quality.getRange('B18').values=[[0]];await wb.recalculate();check('no_treasure_q80',quality.getRange('B20').values[0][0]===80);
quality.getRange('B18').values=[[20]];await wb.recalculate();
await fs.mkdir(path.join(root,'workbook-review'),{recursive:true});
for(const [sheetName,file] of [['配置总览','overview.png'],['品质计算','quality.png'],['天劫计算','tribulation.png']]){
  const blob=await wb.render({sheetName,autoCrop:'all',scale:1.35,format:'png'});
  await fs.writeFile(path.join(root,'workbook-review',file),new Uint8Array(await blob.arrayBuffer()));
}
const report=await wb.inspect({kind:'region',sheetId:'天劫计算',range:'A5:E13',maxChars:2500,tableMaxRows:9,tableMaxCols:5});
await fs.writeFile(path.join(root,'workbook-review/inspect.txt'),report.ndjson??JSON.stringify(report));
const output=await SpreadsheetFile.exportXlsx(wb);
const out=path.join(root,'武灵修仙_物品与突破配置_v1.4.xlsx');
await output.save(out);
await fs.writeFile(path.join(root,'workbook-review/validation.json'),JSON.stringify({passed:true,formulaChecks,worksheetCount:18,authoringLibrary:'@oai/artifact-tool',runtimeTests:false},null,2));
console.log(JSON.stringify({output:out,formulaChecks,worksheetCount:18}));
