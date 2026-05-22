import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore
import QtQuick.Dialogs
import PuzzleSolver

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 720
    title: "Daily Calendar Puzzle Solver"

    // ── Theme ─────────────────────────────────────────────────────────────────
    readonly property bool isDark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark
    Material.theme:  isDark ? Material.Dark  : Material.Light
    Material.accent: Material.Blue

    SolverBackend { id: backend }

    // ── Piece set file picker ─────────────────────────────────────────────────
    FileDialog {
        id: pieceSetFileDialog
        title: "ピースセット JSON を選択"
        nameFilters: ["JSONファイル (*.json)", "すべてのファイル (*)"]
        onAccepted: backend.importPieceSet(selectedFile)
    }

    // Import result notification (Snackbar style)
    Popup {
        id: importToast
        property string message: ""
        property bool success: true

        parent: Overlay.overlay
        width: Math.min(parent.width - 32, 400)
        height: toastLabel.implicitHeight + 24
        x: (parent.width  - width)  / 2
        y:  parent.height - height - 24
        padding: 12
        modal: false
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: importToast.success ? "#323232" : Material.color(Material.Red)
            radius: 6
        }

        Label {
            id: toastLabel
            anchors.fill: parent
            text: importToast.message
            color: "white"
            font.pixelSize: 13
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Timer {
            running: importToast.visible
            interval: 3000
            onTriggered: importToast.close()
        }
    }

    // React to import signals from backend
    property bool importSlideshowActive: backend.slideshow  // reuse existing pattern
    Connections {
        target: backend
        function onImportSucceeded(description) {
            importToast.success = true
            importToast.message = "「" + description + "」を追加しました"
            importToast.open()
        }
        function onImportFailed(error) {
            importToast.success = false
            importToast.message = "読み込みエラー: " + error
            importToast.open()
        }
    }

    // ── Persistent settings ───────────────────────────────────────────────────
    Settings {
        id: appSettings
        category: "PuzzleSolver"
        property bool userFindAll:  false
        property bool autoMidnight: false
        property bool slideshow:    false
    }

    // ── Settings state ────────────────────────────────────────────────────────
    property bool userFindAll:  false
    property bool autoMidnight: false

    // Auto-save whenever these properties change
    onUserFindAllChanged:  appSettings.userFindAll  = userFindAll
    onAutoMidnightChanged: appSettings.autoMidnight = autoMidnight

    // Mirror backend.slideshow so onXxxChanged fires reliably (Connections can silently fail)
    property bool slideshowActive: backend.slideshow
    onSlideshowActiveChanged: {
        appSettings.slideshow = slideshowActive   // persist
        if (slideshowActive) {
            findAllItem.checked = true   // checked BEFORE disabling
            findAllItem.enabled = false
        } else {
            findAllItem.enabled = true
            findAllItem.checked = root.userFindAll
        }
    }

    // ── Date state ────────────────────────────────────────────────────────────
    property int curYear:  Qt.formatDate(new Date(), "yyyy") * 1
    property int curMonth: Qt.formatDate(new Date(), "M") * 1
    property int curDay:   Qt.formatDate(new Date(), "d") * 1

    readonly property bool isToday: {
        var d = new Date()
        return curYear === d.getFullYear() && curMonth === (d.getMonth() + 1) && curDay === d.getDate()
    }

    function curDateStr() {
        var d = new Date(curYear, curMonth - 1, curDay)
        return Qt.formatDate(d, "yyyy/MM/dd (ddd)")
    }

    function addDays(n) {
        var d = new Date(curYear, curMonth - 1, curDay)
        d.setDate(d.getDate() + n)
        if (d < new Date(1900, 0, 1) || d > new Date(2099, 11, 31)) return
        curYear  = d.getFullYear()
        curMonth = d.getMonth() + 1
        curDay   = d.getDate()
        scheduleSolve()
    }

    function goToday() {
        var d = new Date()
        curYear  = d.getFullYear()
        curMonth = d.getMonth() + 1
        curDay   = d.getDate()
        scheduleSolve()
    }

    function scheduleSolve() { debounce.restart() }

    // ── Swipe animation ───────────────────────────────────────────────────────
    function swipeSolution(goingNext) {
        var cx = boardArea.bx   // centred x inside boardArea

        outgoingBoard.boardData   = backend.boardData
        outgoingBoard.boardLabels = backend.boardLabels
        outgoingBoard.x = cx
        outgoingBoard.visible = true

        boardCanvas.x = goingNext ? boardArea.width : -boardArea.bw

        if (goingNext) backend.nextSolution()
        else           backend.prevSolution()

        outgoingAnim.to = goingNext ? -boardArea.bw : boardArea.width
        outgoingAnim.restart()

        incomingAnim.from = boardCanvas.x
        incomingAnim.to   = cx
        incomingAnim.restart()
    }

    Timer {
        id: debounce
        interval: 300
        repeat: false
        onTriggered: backend.solve(curYear, curMonth, curDay,
                                   backend.slideshow || root.userFindAll)
    }

    // ── 深夜自動更新 ──────────────────────────────────────────────────────────
    Timer {
        id: midnightTimer
        repeat: false
        onTriggered: {
            if (root.autoMidnight) root.goToday()
            root.scheduleMidnight()
        }
    }

    function scheduleMidnight() {
        var now      = new Date()
        var midnight = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1, 0, 0, 1)
        midnightTimer.interval = midnight - now
        midnightTimer.restart()
    }

    Component.onCompleted: {
        // Restore saved settings
        root.userFindAll  = appSettings.userFindAll
        root.autoMidnight = appSettings.autoMidnight
        if (appSettings.slideshow) backend.slideshow = true  // triggers onSlideshowActiveChanged

        scheduleSolve()
        scheduleMidnight()
        backend.checkIncomingIntent()  // import piece set if launched via file/share
    }

    // ── Main layout ───────────────────────────────────────────────────────────
    ColumnLayout {
        anchors { fill: parent; margins: 8 }
        spacing: 6

        // Single header row: date label + ⚙
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            // Balances the ⚙ button so the date label is truly centred
            Item { implicitWidth: gearButton.width; implicitHeight: 1 }

            Label {
                id: dateLabel
                Layout.fillWidth: true
                text: curDateStr()
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 16
                font.bold: true

                // Pulse feedback when long-pressing to go to today
                SequentialAnimation {
                    id: todayPulse
                    NumberAnimation {
                        target: dateLabel; property: "scale"
                        to: 1.08; duration: 100; easing.type: Easing.OutQuad
                    }
                    NumberAnimation {
                        target: dateLabel; property: "scale"
                        to: 1.0;  duration: 300; easing.type: Easing.OutBounce
                    }
                }

                ToolTip {
                    id: todayTip
                    text: "今日へ移動"
                    delay: 0
                    timeout: 1200
                }

                MouseArea {
                    anchors.fill: parent
                    property real startX: 0
                    property real startY: 0
                    property bool didLongPress: false

                    onPressed:      { startX = mouseX; startY = mouseY; didLongPress = false }
                    onPressAndHold: {
                        didLongPress = true   // always suppress tap even when already today
                        if (root.isToday) return
                        goToday()
                        todayPulse.restart()
                        todayTip.open()
                    }
                    onReleased: {
                        if (didLongPress) return
                        var dx = mouseX - startX
                        var dy = mouseY - startY
                        if (Math.abs(dx) > Math.abs(dy) * 1.5 && Math.abs(dx) > 40)
                            addDays(dx < 0 ? 1 : -1)
                        else if (Math.sqrt(dx*dx + dy*dy) < 10)
                            datePickerDialog.open()
                    }
                }
            }

            RoundButton {
                id: gearButton
                text: "⚙"
                flat: true
                font.pixelSize: 20
                onClicked: settingsMenu.open()

                Menu {
                    id: settingsMenu

                    MenuItem {
                        id: findAllItem
                        text: "全解探索"
                        checkable: true
                        checked: root.userFindAll
                        onToggled: { root.userFindAll = checked; scheduleSolve() }
                    }

                    MenuItem {
                        text: "スライドショー"
                        checkable: true
                        checked: backend.slideshow
                        onToggled: { backend.slideshow = checked; scheduleSolve() }
                    }

                    MenuItem {
                        text: "深夜自動更新"
                        checkable: true
                        checked: root.autoMidnight
                        onToggled: root.autoMidnight = checked
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: "ピースセットを読み込む…"
                        onTriggered: pieceSetFileDialog.open()
                    }

                    Repeater {
                        model: backend.pieceSetNames
                        MenuItem {
                            text: modelData
                            checkable: true
                            checked: index === backend.currentPieceSet
                            onTriggered: { backend.currentPieceSet = index; scheduleSolve() }
                        }
                    }
                }
            }
        }

        // Board
        Item {
            id: boardArea
            Layout.fillWidth: true
            Layout.fillHeight: true   // take all remaining height
            clip: true

            // Largest board fitting within both width and height
            readonly property real bw: Math.min(width, height * 7 / 8)
            readonly property real bh: bw * 8 / 7
            readonly property real bx: (width  - bw) / 2   // centred horizontally
            readonly property real by: (height - bh) / 2   // centred vertically

            // Outgoing board — shows old solution sliding away
            BoardCanvas {
                id: outgoingBoard
                width: boardArea.bw; height: boardArea.bh
                x: boardArea.bx;     y: boardArea.by
                visible: false
                darkMode: root.isDark

                NumberAnimation on x {
                    id: outgoingAnim
                    duration: 280
                    easing.type: Easing.InCubic
                    onFinished: outgoingBoard.visible = false
                }
            }

            // Main board — current solution, slides in during swipe
            BoardCanvas {
                id: boardCanvas
                width: boardArea.bw; height: boardArea.bh
                x: boardArea.bx;     y: boardArea.by
                boardData:   backend.boardData
                boardLabels: backend.boardLabels
                darkMode:    root.isDark

                NumberAnimation on x {
                    id: incomingAnim
                    duration: 280
                    easing.type: Easing.OutCubic
                    // Re-establish binding so rotation updates x automatically
                    onFinished: boardCanvas.x = Qt.binding(function() { return boardArea.bx })
                }
            }

            // Horizontal swipe to navigate solutions
            MouseArea {
                anchors.fill: parent
                enabled: backend.solutionCount > 1 && !backend.solving
                         && !incomingAnim.running && !outgoingAnim.running
                property real startX: 0
                property real startY: 0

                onPressed: { startX = mouseX; startY = mouseY }
                onReleased: {
                    var dx = mouseX - startX
                    var dy = mouseY - startY
                    if (Math.abs(dx) > Math.abs(dy) * 1.5 && Math.abs(dx) > 40) {
                        root.swipeSolution(dx < 0)   // left = next, right = prev
                    }
                }
            }

            // Solving overlay
            Rectangle {
                anchors.fill: parent
                visible: backend.solving
                color: "#88000000"
                radius: 6

                Column {
                    anchors.centerIn: parent
                    spacing: 16

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: backend.solving
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "解を計算中…"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "キャンセル"
                        Material.background: Material.Red
                        Material.foreground: "white"
                        onClicked: backend.cancel()
                    }
                }
            }
        }

        // Solution label (◀/▶ removed — use board swipe to navigate)
        Label {
            Layout.fillWidth: true
            visible: backend.solutionCount > 0 || backend.solLabel !== ""
            text: backend.solLabel
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
        }

        // Status bar
        Label {
            Layout.fillWidth: true
            visible: backend.statusText !== ""
            text: backend.statusText
            font.pixelSize: 11
            color: Material.hintTextColor
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }

    // ── Date picker dialog (Tumbler wheel) ───────────────────────────────────
    Dialog {
        id: datePickerDialog
        title: "日付を選択"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        // Returns the number of days in the given month/year
        function daysInMonth(year, month) {
            return new Date(year, month, 0).getDate()
        }

        onAccepted: {
            var y = 1900 + yearTumbler.currentIndex
            var m = monthTumbler.currentIndex + 1
            var d = Math.min(dayTumbler.currentIndex + 1, daysInMonth(y, m))
            curYear  = y
            curMonth = m
            curDay   = d
            scheduleSolve()
        }

        onAboutToShow: {
            yearTumbler.currentIndex  = curYear - 1900
            monthTumbler.currentIndex = curMonth - 1
            dayTumbler.currentIndex   = curDay - 1
        }

        Column {
            spacing: 4

            // Column headers
            Row {
                spacing: 0
                Label { width: yearTumbler.width;  text: "年"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12; opacity: 0.7 }
                Label { width: monthTumbler.width; text: "月"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12; opacity: 0.7 }
                Label { width: dayTumbler.width;   text: "日"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12; opacity: 0.7 }
            }

            Row {
                spacing: 0

                Tumbler {
                    id: yearTumbler
                    width: 90; height: 180
                    model: {
                        var a = []
                        for (var y = 1900; y <= 2099; y++) a.push(y)
                        return a
                    }
                    wrap: false
                    delegate: tumblerDelegate
                }

                Tumbler {
                    id: monthTumbler
                    width: 60; height: 180
                    model: 12
                    delegate: tumblerDelegate
                }

                Tumbler {
                    id: dayTumbler
                    width: 60; height: 180
                    model: 31
                    delegate: tumblerDelegate
                }
            }
        }

        // Shared delegate: fade by distance from centre; invalid days grayed out
        Component {
            id: tumblerDelegate
            Label {
                required property var modelData
                required property int index

                // True when this item is a valid day for the selected month/year
                readonly property bool validDay: {
                    if (Tumbler.tumbler !== dayTumbler) return true
                    var y = 1900 + yearTumbler.currentIndex
                    var m = monthTumbler.currentIndex + 1
                    return (index + 1) <= datePickerDialog.daysInMonth(y, m)
                }

                text: {
                    if (Tumbler.tumbler === yearTumbler) return modelData
                    return String(index + 1).padStart(2, "0")
                }
                font.pixelSize: 18
                font.bold: validDay && Math.abs(Tumbler.displacement) < 0.5
                opacity: {
                    var base = Math.max(0.2, 1.0 - Math.abs(Tumbler.displacement)
                                                  / (Tumbler.tumbler.visibleItemCount / 2))
                    return validDay ? base : base * 0.25
                }
                color: validDay ? Material.foreground : Material.hintTextColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
