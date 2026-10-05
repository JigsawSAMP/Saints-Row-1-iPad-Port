import SwiftUI

struct ContentView: View {
    @StateObject private var input = ControllerInput()
    @Environment(\.scenePhase) private var phase
    var body: some View {
        ZStack {
            GameRenderView().ignoresSafeArea()
            if input.runtimeStatus != "Saints Row guest thread running" {
                Text(input.runtimeStatus)
                    .font(.headline).multilineTextAlignment(.center)
                    .padding(24).background(.black.opacity(0.75), in: RoundedRectangle(cornerRadius: 12))
                    .allowsHitTesting(false)
            }
        }
        .preferredColorScheme(.dark)
        .statusBarHidden(true)
        .persistentSystemOverlays(.hidden)
        .onChange(of: phase) { _, value in input.setActive(value == .active) }
        .onAppear { input.setActive(phase == .active) }
        .onDisappear { input.setActive(false) }
    }
}

private struct GameRenderView: UIViewRepresentable {
    func makeUIView(context: Context) -> UIView {
        let view = UIView()
        view.backgroundColor = .black
        SRNativeAttachView(Unmanaged.passUnretained(view).toOpaque())
        DispatchQueue.main.async {
            let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].appendingPathComponent("game")
            directory.path.withCString { SRNativeStart($0) }
        }
        return view
    }
    func updateUIView(_ view: UIView, context: Context) {}
}
