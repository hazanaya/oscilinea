// Renders three Oscilinea feature images for the website: traces in the
// plug-in's drawing style on dark and light backgrounds.
import AppKit

let outDir = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "."
let W: CGFloat = 1600, H: CGFloat = 1200

func render(name: String, bgTop: NSColor, bgBottom: NSColor, trace: NSColor,
            glowAlpha: CGFloat, wave: (CGFloat) -> CGFloat) {
    let image = NSImage(size: NSSize(width: W, height: H))
    image.lockFocus()
    NSGradient(colors: [bgTop, bgBottom])!.draw(in: NSRect(x: 0, y: 0, width: W, height: H), angle: 90)

    let path = NSBezierPath()
    path.lineWidth = 14
    path.lineCapStyle = .round
    path.lineJoinStyle = .round
    let x0: CGFloat = 140, x1: CGFloat = W - 140, midY = H / 2
    let steps = 1200
    for i in 0...steps {
        let t = CGFloat(i) / CGFloat(steps)
        let x = x0 + (x1 - x0) * t
        let y = midY + wave(t)
        if i == 0 { path.move(to: NSPoint(x: x, y: y)) } else { path.line(to: NSPoint(x: x, y: y)) }
    }
    if glowAlpha > 0 {
        NSGraphicsContext.current?.saveGraphicsState()
        let glow = NSShadow()
        glow.shadowColor = trace.withAlphaComponent(glowAlpha)
        glow.shadowBlurRadius = 34
        glow.set()
        trace.withAlphaComponent(0.9).setStroke()
        path.stroke()
        NSGraphicsContext.current?.restoreGraphicsState()
    }
    trace.setStroke()
    path.stroke()
    image.unlockFocus()

    guard let tiff = image.tiffRepresentation, let rep = NSBitmapImageRep(data: tiff),
          let png = rep.representation(using: .png, properties: [:]) else { fatalError("render failed") }
    try! png.write(to: URL(fileURLWithPath: "\(outDir)/\(name).png"))
    print("wrote \(name).png")
}

let nearBlackTop = NSColor(calibratedRed: 0.043, green: 0.043, blue: 0.059, alpha: 1)
let nearBlackBottom = NSColor(calibratedRed: 0.102, green: 0.102, blue: 0.133, alpha: 1)

// 1. The signature wave packet, white on dark (dark-mode default look).
render(name: "oscilinea-feature-shape", bgTop: nearBlackTop, bgBottom: nearBlackBottom,
       trace: .white, glowAlpha: 0.5) { t in
    let env = exp(-pow(t - 0.5, 2) / (2 * pow(0.19, 2)))
    return 340 * env * sin(2 * .pi * 3 * t)
}

// 2. A harmonic-rich synth cycle, soft cyan on dark (color styling).
render(name: "oscilinea-feature-capture", bgTop: nearBlackTop, bgBottom: nearBlackBottom,
       trace: NSColor(calibratedRed: 0.55, green: 0.86, blue: 0.92, alpha: 1), glowAlpha: 0.55) { t in
    let p = 2 * .pi * 2 * t
    return 210 * sin(p) + 105 * sin(3 * p + 0.6) + 48 * sin(5 * p + 1.1)
}

// 3. Light mode: dark neutral trace on a light background, PWM-like shape.
render(name: "oscilinea-feature-look", bgTop: NSColor(calibratedWhite: 0.965, alpha: 1),
       bgBottom: NSColor(calibratedWhite: 0.90, alpha: 1),
       trace: NSColor(calibratedRed: 0.16, green: 0.16, blue: 0.19, alpha: 1), glowAlpha: 0) { t in
    let p = 2 * .pi * 2.5 * t
    return 280 * tanh(3.2 * sin(p + 0.9 * sin(2 * p)))
}
