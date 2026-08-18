import QtQuick
import QtQuick.Window

// MKL 主窗口（空白窗口骨架）
// 规格：模块 19 MainWindow —— 当前为最小实现：标题栏占位 + 内容区占位
Window {
    id: root
    width: 960
    height: 600
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: qsTr("麦块启动器 MKL v0.0.1-alpha")
    color: "#1e1e2e"

    // 标题栏占位（规格：模块 20 TitleBar，M2 实现完整版）
    Rectangle {
        id: titleBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 36
        color: "#11111b"
        Text {
            anchors.centerIn: parent
            color: "#cdd6f4"
            font.pixelSize: 13
            text: root.title
        }
    }

    // 内容区占位
    Text {
        anchors.centerIn: parent
        color: "#a6adc8"
        font.pixelSize: 16
        text: qsTr("MKL 空白窗口骨架\n基础层模块已就绪，UI/服务模块随里程碑推进")
        horizontalAlignment: Text.AlignHCenter
    }
}
