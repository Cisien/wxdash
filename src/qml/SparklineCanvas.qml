import QtQuick

Canvas {
    id: root

    property var data: []
    property real rangeMin: 0
    property real rangeMax: 100

    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onDataChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        var d = data
        if (!d || d.length < 2) return

        var count = d.length
        var slHeight = height / 3
        var slTop = height - slHeight
        var stride = Math.max(1, Math.floor(count / width))

        var dataMin = d[0], dataMax = d[0]
        for (var i = 1; i < count; i++) {
            if (d[i] < dataMin) dataMin = d[i]
            if (d[i] > dataMax) dataMax = d[i]
        }

        var minV = rangeMin, maxV = rangeMax
        if (dataMin < minV) minV = dataMin
        if (dataMax > maxV) maxV = dataMax
        var range = maxV - minV
        if (range === 0) range = 1

        ctx.beginPath()
        ctx.strokeStyle = "#5A4500"
        ctx.lineWidth = 1
        ctx.lineJoin = "round"

        var first = true
        for (var j = 0; j < count; j += stride) {
            var x = (j / (count - 1)) * width
            var y = slTop + slHeight - ((d[j] - minV) / range) * slHeight * 0.9
            if (first) { ctx.moveTo(x, y); first = false }
            else ctx.lineTo(x, y)
        }
        ctx.stroke()
    }
}
