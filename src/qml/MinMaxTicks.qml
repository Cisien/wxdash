import QtQuick

Canvas {
    id: root

    property real trueMin: NaN
    property real trueMax: NaN
    property real rangeMin: 0
    property real rangeMax: 100
    property real arcRadius: 0
    property real arcStrokeWidth: 0
    property real arcStartAngle: 125
    property real arcSweepAngle: 290

    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onTrueMinChanged: requestPaint()
    onTrueMaxChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        if (isNaN(trueMin) || isNaN(trueMax)) return
        if (rangeMax === rangeMin) return

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

        drawTick(trueMin, "#5B8DD9")
        drawTick(trueMax, "#C84040")
    }
}
