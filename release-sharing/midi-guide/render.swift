import AppKit
import CoreGraphics
import Foundation

// Run from cosmos-software. The guide uses the same language codes as both products.
let base = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
let source = base.appendingPathComponent("release-sharing/midi-guide/copy.txt")
let output = base.appendingPathComponent("output/pdf/MIDI Map")
try FileManager.default.createDirectory(at: output, withIntermediateDirectories: true)
let list = try JSONDecoder().decode([[String:String]].self, from: Data(contentsOf: base.appendingPathComponent("app/assets/interface_languages.json")))
struct LogicPage: Decodable {let titles:[String];let body:[String]}
let logicPages = try JSONDecoder().decode([String:LogicPage].self,from:Data(contentsOf:base.appendingPathComponent("app/assets/logic_midi_guide.json")))
let lines = try String(contentsOf: source, encoding: .utf8).components(separatedBy: .newlines)
var copy = [String:[String]](), key = ""
for line in lines where !line.isEmpty {
    if line.hasPrefix("[") { key = String(line.dropFirst().dropLast()); copy[key] = [] }
    else { copy[key, default: []].append(line) }
}
precondition(Set(list.compactMap { $0["code"] }) == Set(copy.keys))
let examples = ["zh-Hans":"示例","zh-Hant":"範例","en":"Example","ja":"設定例","es":"Ejemplo","fr":"Exemple","de":"Beispiel","th":"ตัวอย่าง","id":"Contoh","hi":"उदाहरण","vi":"Ví dụ","it":"Esempio","pt":"Exemplo","ar":"مثال","ko":"예시"]
let W:CGFloat=595, H:CGFloat=842
func color(_ hex:Int) -> NSColor { NSColor(srgbRed:CGFloat((hex>>16)&255)/255,green:CGFloat((hex>>8)&255)/255,blue:CGFloat(hex&255)/255,alpha:1) }
let paper=color(0xeaf0e3), ink=color(0x172e29), teal=color(0x295e4e), muted=color(0x445e53), rule=color(0xa4b5a3), pale=color(0xdce7d7), yellow=color(0xf5bd40)
func pos(_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat)->NSRect { NSRect(x:x,y:H-y-h,width:w,height:h) }
func fill(_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat,_ c:NSColor){ c.setFill(); NSBezierPath(rect:pos(x,y,w,h)).fill() }
func circle(_ x:CGFloat,_ y:CGFloat,_ radius:CGFloat,_ c:NSColor, outline:Bool=false){
    let path=NSBezierPath(ovalIn:pos(x-radius,y-radius,2*radius,2*radius))
    if outline { c.setStroke(); path.lineWidth=0.9; path.stroke() } else { c.setFill();path.fill() }
}
var currentLanguage = ""
var pageText = [String]()
func text(_ s:String,_ x:CGFloat,_ y:CGFloat,_ w:CGFloat,_ h:CGFloat,_ size:CGFloat,_ c:NSColor=ink,_ bold:Bool=false,_ rtl:Bool=false,_ face:String?=nil,_ center:Bool=false){
    var f=size
    while true {
        let typeface = face ?? (currentLanguage=="ko" ? "AppleGothic":nil)
        let font=typeface.flatMap{NSFont(name:$0,size:f)} ?? NSFont.systemFont(ofSize:f,weight:bold ? .semibold:.regular)
        let p=NSMutableParagraphStyle()
        p.baseWritingDirection=rtl ? .rightToLeft:.leftToRight
        p.alignment=center ? .center:(rtl ? .right:.left)
        p.lineBreakMode = .byWordWrapping
        p.lineSpacing = f>20 ? 0:2.2
        let string=NSAttributedString(string:s,attributes:[.font:font,.foregroundColor:c,.paragraphStyle:p])
        let opts:NSString.DrawingOptions=[.usesLineFragmentOrigin,.usesFontLeading]
        let used=string.boundingRect(with:NSSize(width:w,height:1000),options:opts)
        if used.height<=h+0.2 && used.width<=w+1 {
            pageText.append(s)
            let drawing = NSGraphicsContext.current!.cgContext
            CGPDFContextBeginTag(drawing,.span,[CGPDFTagProperty.actualText:s,CGPDFTagProperty.languageText:currentLanguage] as CFDictionary)
            string.draw(with:pos(x,y,w,h),options:opts)
            CGPDFContextEndTag(drawing)
            break
        }
        f-=0.5
        precondition(f>=size*0.75,"Text does not fit: "+s)
    }
}
func dial(_ cx:CGFloat,_ cy:CGFloat,_ page:Int) {
    circle(cx,cy,30,teal,outline:true)
    for n in 0..<12 {
        let ang = (Double(n)*30-90) * .pi/180, x=cx+CGFloat(cos(ang))*24, y=cy+CGFloat(sin(ang))*24
        circle(x,y,CGFloat(1+n%3)*0.67,n == page+2 ? yellow:teal)
    }
    circle(cx,cy,11.5,ink)
    let a = Double(page==0 ? -62 : 35) * .pi/180
    let hand=NSBezierPath(); hand.move(to:NSPoint(x:cx,y:H-cy)); hand.line(to:NSPoint(x:cx+CGFloat(cos(a))*21,y:H-cy-CGFloat(sin(a))*21));hand.lineWidth=4;hand.lineCapStyle = .round;teal.setStroke();hand.stroke()
}
let logo=NSImage(contentsOf:base.appendingPathComponent("release-sharing/midi-guide/wordmark.png"))!
for item in list {
    let lang=item["code"]!, nm=item["name"]!, rtl=lang=="ar", rows=copy[lang]!
    currentLanguage=lang
    let labels=rows[0].components(separatedBy:"|");precondition(labels.count==11 && rows.count==9)
    let url=output.appendingPathComponent("Cosmos-MIDI-Map-"+lang+".pdf")
    var media=CGRect(x:0,y:0,width:W,height:H)
    let info=[kCGPDFContextTitle:"Cosmos · "+labels[0]+" · "+nm,kCGPDFContextAuthor:"Cosmos"] as CFDictionary
    let ctx=CGContext(url as CFURL, mediaBox:&media, info)!
    for page in 0..<3 {
        pageText=[]
        ctx.beginPDFPage(nil)
        NSGraphicsContext.saveGraphicsState();NSGraphicsContext.current=NSGraphicsContext(cgContext:ctx,flipped:false)
        fill(0,0,W,H,paper)
        let logoBox=pos(40,34,126,24)
        logo.draw(in:logoBox)
        text(nm,327,36,228,23,13,muted,false,true)
        fill(40,75,515,1,teal)
        let markX:CGFloat=rtl ? 85:509
        dial(markX,147,page)
        text(labels[0],rtl ? 150:40,100,405,53,37,ink,true,rtl)
        let subtitle = page == 0 ? labels[1]+"  /  0.16.2" : (page==1 ? "Ableton Live":"Logic Pro")+"  /  Cosmos AU 0.1.10"
        text(subtitle,rtl ? 150:40,157,405,30,16,teal,true,rtl)
        fill(40,211,515,45,pale)
        // Keep arrows and product names in signal-flow order, mirrored for Arabic.
        let route = page==0 ? [labels[2],"Cosmos"] : [labels[2],page==1 ? "Live MIDI Map":"Logic Pro","Cosmos AU"]
        let edge:CGFloat=47; let cell=(W-2*edge)/CGFloat(route.count)
        for (i,node) in route.enumerated(){
            let slot=rtl ? route.count-1-i:i
            text(node,edge+CGFloat(slot)*cell+6,224,cell-12,24,13.5,teal,true,rtl,nil,true)
            if i+1<route.count {let x=edge+(rtl ? CGFloat(slot):CGFloat(slot+1))*cell;text(rtl ? "←":"→",x-12,222,24,26,16,teal,false,false,nil,true)}
        }
        func section(_ n:Int,_ y:CGFloat,_ title:String,_ body:String) {
            let numberX:CGFloat=rtl ? 533:62
            circle(numberX,y+13,12,pale);circle(numberX,y+13,12,rule,outline:true)
            text(String(format:"%02d",n),numberX-14,y+5,28,19,10,teal,true,false,"Menlo-Bold",true)
            let x:CGFloat=rtl ? 40:93
            text(title,x,y-2,462,31,18,ink,true,rtl)
            text(body,x,y+32,462,72,14.0,ink,false,rtl)
            fill(40,y+110,515,0.6,rule)
        }
        let headings = page==2 ? Array(logicPages[lang]!.titles.dropFirst()) : Array(labels[(3+4*page)...(6+4*page)])
        let bodies = page==2 ? logicPages[lang]!.body : Array(rows[(1+4*page)...(4+4*page)])
        for n in 0..<3 {
            section(n+1,285+CGFloat(n)*118,headings[n],bodies[n])
        }
        fill(40,650,515,82,pale)
        text(headings[3],55,657,485,22,13.5,teal,true,rtl)
        text(bodies[3],55,685,485,41,12,ink,false,rtl)
        let technical = "CH 5     RECORD / CC 0      ·      ERASE ALL / CC 1"
        let exX:CGFloat=rtl ? 420:40, techX:CGFloat=rtl ? 40:168
        text(examples[lang]!,exX,753,135,22,12,muted,true,rtl)
        text(technical,techX,752,387,24,11,ink,false,false,"Menlo")
        fill(40,790,515,0.8,rule)
        text("COSMOS    /    MIDI",40,803,280,22,10,teal,false,false,"Menlo")
        text(String(format:"0%d  /  03",page+1),476,803,79,22,10,teal,false,false,"Menlo",true)
        NSGraphicsContext.restoreGraphicsState();ctx.endPDFPage()
        let temp=base.appendingPathComponent("tmp/pdfs/midi-map")
        try FileManager.default.createDirectory(at:temp,withIntermediateDirectories:true)
        try JSONEncoder().encode(pageText).write(to:temp.appendingPathComponent(lang+"-"+String(page)+".json"))
    }
    ctx.closePDF()
    print(lang, url.lastPathComponent)
}
