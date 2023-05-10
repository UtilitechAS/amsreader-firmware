<script>
    export let pct = 0;
    export let color = "red";

    function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
        var angleInRadians = (angleInDegrees-90) * Math.PI / 180.0;

        return {
            x: centerX + (radius * Math.cos(angleInRadians)),
            y: centerY + (radius * Math.sin(angleInRadians))
        };
    }
    function describeArc(x, y, radius, startAngle, endAngle){
       var start = polarToCartesian(x, y, radius, endAngle);
        var end = polarToCartesian(x, y, radius, startAngle);

        var arcSweep = endAngle - startAngle <= 180 ? "0" : "1";

        var d = [
            "M", start.x, start.y, 
            "A", radius, radius, 0, arcSweep, 0, end.x, end.y
        ].join(" ");

        return d;       
    }
</script>

<svg viewBox="0 0 300 300" xmlns="http://www.w3.org/2000/svg" height="100%">
    <path d="{ describeArc(150, 150, 115, 210, 510) }" stroke="#eee" fill="none" stroke-width="55"/>
    <path d="{ describeArc(150, 150, 115, 210, 210 + (300*pct/100)) }" stroke={color} fill="none" stroke-width="55"/>
</svg>
