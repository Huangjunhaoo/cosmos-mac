import AppKit
import Foundation

// Render one Mac installation card per UI language and a language index.
let root = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
let source = root.appendingPathComponent("release-sharing/install-guide/copy.txt")
let out = root.appendingPathComponent("tmp/pdfs/installation")
try FileManager.default.createDirectory(at: out, withIntermediateDirectories: true)
let languages = try JSONDecoder().decode([[String:String]].self, from: Data(contentsOf:root.appendingPathComponent("app/assets/interface_languages.json")))
let logo = NSImage(contentsOf:root.appendingPathComponent("release-sharing/midi-guide/wordmark.png"))!
var lang = "", blocks = [String:[String]]()
for line in try String(contentsOf:source,encoding:.utf8).components(separatedBy:.newlines) where !line.isEmpty {
    if line.hasPrefix("[") { lang=String(line.dropFirst().dropLast()); precondition(blocks[lang]==nil); blocks[lang]=[] }
    else { blocks[lang,default:[]].append(line) }
}
precondition(Set(blocks.keys)==Set(languages.compactMap { $0["code"] }))
let paper=col(0xeaf0e3), ink=col(0x172e29), green=col(0x295e4e), muted=col(0x455e52), lineColor=col(0xa5b6a4), pale=col(0xdbe7d6), amber=col(0xe6b741)
let width:CGFloat=595, height:CGFloat=842, left:CGFloat=41
var runs=[String](), locale=""
func col(_ v:Int)->NSColor { NSColor(srgbRed:CGFloat((v>>16)&255)/255,green:CGFloat((v>>8)&255)/255,blue:CGFloat(v&255)/255,alpha:1) }
func box(_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat)->NSRect {NSRect(x:x,y:height-y-h,width:w,height:h)}
func fill(_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat,_ c:NSColor){c.setFill();NSBezierPath(rect:box(x,y,w,h)).fill()}
func circle(_ x:CGFloat,_ y:CGFloat,_ r:CGFloat,_ c:NSColor){c.setFill();NSBezierPath(ovalIn:box(x-r,y-r,2*r,2*r)).fill()}
enum Align{case natural, start, end, center}
func draw(_ s:String,_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat,_ size:CGFloat,_ color:NSColor=ink,_ bold:Bool=false,_ align:Align = .natural,_ code:Bool=false,_ customLanguage:String?=nil){
    let language=customLanguage ?? locale
    let rtl=language=="ar" && !code
    let style=NSMutableParagraphStyle()
    style.baseWritingDirection=rtl ? .rightToLeft:.leftToRight
    switch align {case .natural:style.alignment=rtl ? .right:.left;case .start:style.alignment = .left;case .end:style.alignment = .right;case .center:style.alignment = .center}
    style.lineSpacing=size>=20 ? 0:1.8
    let opts:NSString.DrawingOptions=[.usesLineFragmentOrigin,.usesFontLeading]
    var f=size
    while true {
        let font:NSFont
        if code {font=NSFont.monospacedSystemFont(ofSize:f, weight:bold ? .semibold:.regular)}
        else if language=="ko",let kr=NSFont(name:"AppleGothic",size:f){font=kr}
        else {font=NSFont.systemFont(ofSize:f,weight:bold ? .semibold:.regular)}
        let text=NSAttributedString(string:s,attributes:[.font:font,.foregroundColor:color,.paragraphStyle:style])
        let bounds=text.boundingRect(with:NSSize(width:w,height:1500),options:opts)
        if bounds.height <= h+0.1 && bounds.width <= w+0.7 {
            runs.append(s)
            let ctx=NSGraphicsContext.current!.cgContext
            CGPDFContextBeginTag(ctx,.span,[CGPDFTagProperty.actualText:s,CGPDFTagProperty.languageText:language] as CFDictionary)
            text.draw(with:box(x,y,w,h),options:opts)
            CGPDFContextEndTag(ctx)
            break
        }
        f-=0.25
        precondition(f>=size*0.81,"Text did not fit: "+s)
    }
}
func sheet(_ id:String,_ title:String,_ body:()->Void) throws {
    runs=[]
    var media=CGRect(x:0,y:0,width:width,height:height)
    let info=[kCGPDFContextTitle:title,kCGPDFContextAuthor:"Cosmos"] as CFDictionary
    let ctx=CGContext(out.appendingPathComponent(id+".pdf") as CFURL,mediaBox:&media,info)!
    ctx.beginPDFPage(nil)
    NSGraphicsContext.saveGraphicsState();NSGraphicsContext.current=NSGraphicsContext(cgContext:ctx,flipped:false)
    fill(0,0,width,height,paper);body()
    NSGraphicsContext.restoreGraphicsState();ctx.endPDFPage();ctx.closePDF()
    try JSONEncoder().encode(runs).write(to:out.appendingPathComponent(id+".json"))
}
func header(_ name:String){
    logo.draw(in:box(left,28,126,24))
    draw(name,230,31,324,23,13,muted,false,.end)
    fill(left,67,513,1,green)
}
func footer(_ id:String){
    fill(left,795,513,0.7,lineColor);draw("COSMOS   /   MAC",left,805,210,16,9.5,green,false,.start,true)
    draw(id,310,804,244,18,10,green,false,.end,true)
}
func heading(_ number:String,_ name:String,_ y:CGFloat){
    let rtl=locale=="ar",nx:CGFloat=rtl ? 540:54,tx:CGFloat=rtl ? 41:81
    circle(nx,y+11,12,pale)
    draw(number,nx-12,y+4,24,16,10,green,true,.center,true)
    draw(name,tx,y-2,473,33,20,ink,true)
}
func pathBar(_ s:String,_ y:CGFloat){
    fill(left,y,513,22,pale)
    draw(s,51,y+3,493,17,11.5,green,false,locale=="ar" ? .end:.start,true)
}
locale="en"
try sheet("Index","Cosmos · Installation guide · 15 languages"){
    header("MAC EDITION     /     15")
    draw("Install Cosmos",left,91,511,60,43,ink,true)
    draw("安装指南   /   Installation guide",left,161,511,37,19,green,true)
    fill(left,227,513,45,pale)
    draw("Standalone  0.16.3     /     AUv2  0.1.12",left+13,239,480,24,14,green,false,.start,true)
    draw("选择语言，跳转至安装说明。",left,298,511,25,16,ink,true)
    draw("Choose a language to open its installation page.",left,326,511,23,14,muted)
    for (i,l) in languages.enumerated(){
        let x=left+CGFloat(i%3)*174,y=382+CGFloat(i/3)*67
        fill(x,y,164,50,pale)
        circle(x+12,y+12,2.5,amber)
        draw(l["name"]!,x+8,y+15,148,29,15.5,green,true,.center,false,l["code"])
    }
    fill(left,741,513,0.7,lineColor)
    draw("macOS 13+",left,756,220,23,13,green,false,.start,true)
    footer("INDEX")
}
for l in languages {
    locale=l["code"]!;let tag=l["name"]!,rows=blocks[locale]!
    precondition(rows.count==8)
    let heads=rows[2].components(separatedBy:"|");precondition(heads.count==5)
    try sheet(locale,"Cosmos · "+rows[0]+" · "+tag){
        header(tag)
        draw(rows[0],left,81,513,54,36,ink,true)
        draw(rows[1],left,139,513,48,14.5,muted)
        heading("01",heads[0],199)
        draw("0.16.3   /   Apple silicon + Intel",left,235,513,17,11,green,false,locale=="ar" ? .end:.start,true)
        draw(rows[3],left,259,513,69,13.6)
        pathBar("Standalone / COSMOS-0.16.3.app",329)
        heading("02",heads[1],372)
        draw("0.1.12   /   Apple silicon + Intel",left,408,513,17,11,green,false,locale=="ar" ? .end:.start,true)
        draw(rows[4],left,429,513,68,13.6)
        pathBar("Audio Unit / Cosmos.component",497)
        pathBar("~/Library/Audio/Plug-Ins/Components/",521)
        heading("03",heads[2],561)
        let rtl=locale=="ar"
        draw("Ableton Live",rtl ? 450:left,598,105,20,12,green,true,rtl ? .end:.start,false,"en")
        draw(rows[5],rtl ? left:154,595,400,46,12.8)
        draw("Logic Pro",rtl ? 450:left,653,105,20,12,green,true,rtl ? .end:.start,false,"en")
        draw(rows[6],rtl ? left:154,650,400,47,12.8)
        fill(left,707,513,82,pale)
        draw(heads[3],52,713,491,19,12.2,green,true)
        draw(rows[7],52,735,491,48,11.5,ink)
        footer(locale.uppercased())
        // The footer link is added to the finished PDF below this page text.
        draw(heads[4]+" ↗",190,805,215,19,10.2,green,true,.center)
    }
    print(locale)
}
