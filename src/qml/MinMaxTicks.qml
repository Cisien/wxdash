import QtQuick

Canvas {
    id: root

    property var data: []
    property real rangeMin: 0
    property real rangeMax: 100
    property real arcRadius: 0
    property real arcStrokeWidth: 0
    property real arcStartAngle: 125
    property real arcSweepAngle: 290

    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onDataChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        var d = data
        if (!d || d.length < 2) return
        if (rangeMax === rangeMin) return

        var dataMin = d[0], dataMax = d[0]
        for (var i = 1; i < d.length; i++) {
            if (d[i] < dataMin) dataMin = d[i]
            if (d[i] > dataMax) dataMax = d[i]
        }

        var cx = width / 2
        var cy = height / 2
        var r = arcRadius
        var halfStroke = arcStrokeWidth / 2

        function drawTick(val, color) {
            var ratio = Math.max(0, Math.min(1, (val - rangeMin) / (rangeMax - rangeMin)))
            var angleDeg = arcStartAngle + arcSweepAngle * ratio
            var angleRad = angleDeg * Math.PI / 180
            var cosA = Math.cos(angleRad)
            var sinA = Math.sin(angleRad)
            ctx.beginPath()
            ctx.strokeStyle = color
            ctx.lineWidth = 2
            ctx.moveTo(cx + (r - halfStroke - 2) * cosA, cy + (r - halfStroke - 2) * sinA)
            ctx.lineTo(cx + (r + halfStroke + 2) * cosA, cy + (r + halfStroke + 2) * sinA)
            ctx.stroke()
        }

        drawTick(dataMin, "#5B8DD9")
        drawTick(dataMax, "#C84040")
    }
}
