// Renders the Oscilinea app icon: a white wave-packet trace on a dark squircle,
// following the macOS Big Sur icon grid (824pt squircle centered on a 1024pt
// transparent canvas). Regenerate with:
//   swift packaging/icon/make-icon.swift packaging/icon/oscilinea-1024.png
import AppKit

let out = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "oscilinea-1024.png"
let size: CGFloat = 1024

let image = NSImage(size: NSSize(width: size, height: size))
image.lockFocus()

// Squircle plate on the Apple icon grid.
let plate = NSRect(x: 100, y: 100, width: 824, height: 824)
let platePath = NSBezierPath(roundedRect: plate, xRadius: 185, yRadius: 185)
NSGradient(colors: [
    NSColor(calibratedRed: 0.055, green: 0.055, blue: 0.075, alpha: 1.0),
    NSColor(calibratedRed: 0.115, green: 0.115, blue: 0.150, alpha: 1.0),
])!.draw(in: platePath, angle: 90)

// Wave packet: a sine carrier under a Gaussian envelope, the "sound shape".
let trace = NSBezierPath()
trace.lineWidth = 30
trace.lineCapStyle = .round
trace.lineJoinStyle = .round
let x0: CGFloat = 196, x1: CGFloat = 828, midY: CGFloat = 512
let cycles: CGFloat = 3.0
let steps = 600
for i in 0...steps {
    let t = CGFloat(i) / CGFloat(steps)
    let x = x0 + (x1 - x0) * t
    let envelope = exp(-pow(t - 0.5, 2) / (2 * pow(0.17, 2)))
    let y = midY + 240 * envelope * sin(2 * .pi * cycles * t)
    if i == 0 { trace.move(to: NSPoint(x: x, y: y)) }
    else { trace.line(to: NSPoint(x: x, y: y)) }
}

// Soft glow pass, then the crisp trace on top.
NSGraphicsContext.current?.saveGraphicsState()
let glow = NSShadow()
glow.shadowColor = NSColor(calibratedWhite: 1.0, alpha: 0.55)
glow.shadowBlurRadius = 42
glow.set()
NSColor(calibratedWhite: 1.0, alpha: 0.9).setStroke()
trace.stroke()
NSGraphicsContext.current?.restoreGraphicsState()
NSColor.white.setStroke()
trace.stroke()

image.unlockFocus()

guard let tiff = image.tiffRepresentation,
      let rep = NSBitmapImageRep(data: tiff),
      let png = rep.representation(using: .png, properties: [:]) else {
    fputs("failed to render PNG\n", stderr)
    exit(1)
}
try! png.write(to: URL(fileURLWithPath: out))
print("wrote \(out)")
