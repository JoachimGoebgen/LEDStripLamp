// FULL CREDITS GO OUT TO markE/m1erickson FOR MAKING THIS SNIPPET.
// SEE http://jsfiddle.net/m1erickson/Nfs9R/

// Add a rainboxRect function to the context prototype
// This method is used alone like context.fillRect
// This method is not used within a context.beginPath
// NOTE: this addition must always be run before it is used in code
CanvasRenderingContext2D.prototype.rainbowRect = function (x, y, w, h, fillColor, tColor, rColor, bColor, lColor) {

    // use existing fillStyle if fillStyle is not supplied
    fillColor = fillColor || this.fillStyle;

    // use existing strokeStyle if any strokeStyle is not supplied
    var ss = this.strokeStyle;
    tColor = tColor || ss;
    rColor = rColor || ss;
    bColor = bColor || ss;
    lColor = lColor || ss;


    // context will be modified, so save it
    this.save();

    // miter the lines
    this.lineJoin = "miter";

    // helper function: draws one side's trapzoidal "stroke"
    function trapezoid(context, color, x1, y1, x2, y2, x3, y3, x4, y4) {
        context.beginPath();
        context.moveTo(x1, y1);
        context.lineTo(x2, y2);
        context.lineTo(x3, y3);
        context.lineTo(x4, y4);
        context.closePath();
        context.fillStyle = color;
        context.fill();
    }

    // context lines are always drawn half-in/half-out
    // so context.lineWidth/2 is used a lot
    var lw = this.lineWidth / 2;

    // shortcut vars for boundaries
    var L = x - lw;
    var R = x + lw;
    var T = y - lw;
    var B = y + lw;

    // top
    trapezoid(this, tColor, L, T, R + w, T, L + w, B, R, B);

    // right
    trapezoid(this, rColor, R + w, T, R + w, B + h, L + w, T + h, L + w, B);

    // bottom
    trapezoid(this, bColor, R + w, B + h, L, B + h, R, T + h, L + w, T + h);

    // left
    trapezoid(this, lColor, L, B + h, L, T, R, B, R, T + h);

    // fill
    this.fillStyle = fillColor;
    this.fillRect(x, y, w, h);

    // be kind -- always rewind (old vhs reference!)
    this.restore();
    // don't let this path leak
    this.beginPath();
    // chain
    return (this);
};


