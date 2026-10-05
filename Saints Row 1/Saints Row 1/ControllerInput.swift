import SwiftUI
import Combine
@preconcurrency import GameController

@MainActor
final class ControllerInput: NSObject, ObservableObject {
    @Published private(set) var runtimeStatus = "Starting Saints Row"
    private var link: CADisplayLink?
    private let target = DisplayLinkTarget()
    private weak var boundPad: GCExtendedGamepad?
    private var active = true
    private var lastStatusPoll: CFTimeInterval = 0

    override init() {
        super.init()
        target.owner = self
        let display = CADisplayLink(target: target, selector: #selector(DisplayLinkTarget.tick))
        display.preferredFrameRateRange = CAFrameRateRange(minimum: 60, maximum: 120, preferred: 120)
        display.add(to: .main, forMode: .common)
        link = display
    }
    func setActive(_ value: Bool) {
        active = value
        SRNativeSetActive(value ? 1 : 0)
        link?.isPaused = !value
        SRInputSetControllerActive(value ? 1 : 0)
        SRInputReset()
        SRMobileClearLook()
        if !value { boundPad?.valueChangedHandler = nil; boundPad = nil }
        if value { pollController() }
    }
    nonisolated private static func state(_ pad: GCExtendedGamepad) -> SRInputState {
        var state = SRInputState()
        let buttons: [(GCControllerButtonInput?, UInt16)] = [
            (pad.dpad.up, 0x0001), (pad.dpad.down, 0x0002), (pad.dpad.left, 0x0004), (pad.dpad.right, 0x0008),
            (pad.buttonMenu, 0x0010), (pad.buttonOptions, 0x0020),
            (pad.leftThumbstickButton, 0x0040), (pad.rightThumbstickButton, 0x0080),
            (pad.leftShoulder, 0x0100), (pad.rightShoulder, 0x0200),
            (pad.buttonA, 0x1000), (pad.buttonB, 0x2000), (pad.buttonX, 0x4000), (pad.buttonY, 0x8000)
        ]
        for (button, mask) in buttons where button?.isPressed == true { state.buttons |= mask }
        state.leftTrigger = UInt8((min(1, max(0, pad.leftTrigger.value)) * 255).rounded())
        state.rightTrigger = UInt8((min(1, max(0, pad.rightTrigger.value)) * 255).rounded())
        func axis(_ value: Float) -> Int16 { let v = min(1, max(-1, value)); return Int16((v * (v < 0 ? 32768 : 32767)).rounded()) }
        state.leftX = axis(pad.leftThumbstick.xAxis.value); state.leftY = axis(pad.leftThumbstick.yAxis.value)
        state.rightX = axis(pad.rightThumbstick.xAxis.value); state.rightY = axis(pad.rightThumbstick.yAxis.value)
        return state
    }
    private func pollController() {
        let controller = GCController.current ?? GCController.controllers().first { $0.extendedGamepad != nil }
        let pad = controller?.extendedGamepad
        if boundPad !== pad {
            boundPad?.valueChangedHandler = nil
            boundPad = pad
            // Publish directly from hardware events; no render-frame wait is needed.
            pad?.valueChangedHandler = { pad, _ in SRInputSetController(Self.state(pad)) }
        }
        if active, let pad { SRInputSetController(Self.state(pad)) }
        else { SRInputSetController(SRInputState()) }
    }
    fileprivate func poll() {
        pollController()
        let now = CACurrentMediaTime()
        if now - lastStatusPoll > 0.25 {
            let status = String(cString: SRNativeStatus())
            if status != runtimeStatus { runtimeStatus = status }
            lastStatusPoll = now
        }
        SRNativePumpEvents()
    }
}

@MainActor
private final class DisplayLinkTarget: NSObject {
    weak var owner: ControllerInput?
    @objc func tick() { owner?.poll() }
}
