/**
 *	miniRT prim CommonJS module
 *	(c) 2015-2017 Fat Cerberus
**/

'use strict';

const Prim =
{
	blit(surface, x, y, texture, mask)
	{
		Prim.blitSection(surface, x, y, texture, 0, 0, texture.width, texture.height, mask);
	},

	blitSection(surface, x, y, texture, sx, sy, width, height, mask)
	{
		mask = mask || Color.White;

		var x1 = x;
		var y1 = y;
		var x2 = x1 + width;
		var y2 = y1 + height;
		var u1 = sx / texture.width;
		var v1 = 1.0 - sy / texture.height;
		var u2 = (sx + width) / texture.width;
		var v2 = 1.0 - (sy + height) / texture.height;
		var shape = new Shape(ShapeType.TriStrip, texture,
			new VertexList([
				{ x: x1, y: y1, u: u1, v: v1, color: mask },
				{ x: x2, y: y1, u: u2, v: v1, color: mask },
				{ x: x1, y: y2, u: u1, v: v2, color: mask },
				{ x: x2, y: y2, u: u2, v: v2, color: mask },
			]));
		shape.draw(surface);
	},

	drawCircle(surface, x, y, radius, color)
	{
		Prim.drawEllipse(surface, x, y, radius, radius, color);
	},

	drawEllipse(surface, x, y, rx, ry, color)
	{
		var numSegments = Math.ceil(10 * Math.sqrt((rx + ry) / 2.0));
		var vertices = [];
		var tau = 2 * Math.PI;
		var cos = Math.cos;
		var sin = Math.sin;
		for (var i = 0; i < numSegments - 1; ++i) {
			var phi = tau * i / numSegments;
			var c = cos(phi);
			var s = sin(phi);
			vertices.push({
				x: x + c * rx,
				y: y - s * ry,
				color: color
			});
		}
		var vList = new VertexList(vertices);
		var shape = new Shape(ShapeType.LineLoop, vList)
		shape.draw(surface);
	},

	drawSolidCircle(surface, x, y, radius, color, color2)
	{
		Prim.drawSolidEllipse(surface, x, y, radius, radius, color, color2);
	},

	drawSolidEllipse(surface, x, y, rx, ry, color, color2)
	{
		color2 = color2 || color;

		var numSegments = Math.ceil(10 * Math.sqrt((rx + ry) / 2.0));
		var vertices = [ { x: x, y: y, color: color } ];
		var tau = 2 * Math.PI;
		var cos = Math.cos;
		var sin = Math.sin;
		for (var i = 0; i < numSegments; ++i) {
			var phi = tau * i / numSegments;
			var c = cos(phi);
			var s = sin(phi);
			vertices[i + 1] = {
				x: x + c * rx,
				y: y - s * ry,
				color: color2
			};
		}
		vertices[numSegments + 1] = {
			x: x + rx,  // cos(0) = 1.0
			y: y,       // sin(0) = 0.0
			color: color2
		};

		var vList = new VertexList(vertices);
		var shape = new Shape(ShapeType.Fan, vList);
		shape.draw(surface);
	},

	drawSolidRectangle(surface, x, y, width, height, color_ul, color_ur, color_lr, color_ll)
	{
		color_ur = color_ur || color_ul;
		color_lr = color_lr || color_ul;
		color_ll = color_ll || color_ul;

		var shape = new Shape(ShapeType.TriStrip,
			new VertexList([
				{ x: x, y: y, color: color_ul },
				{ x: x + width, y: y, color: color_ur },
				{ x: x, y: y + height, color: color_ll },
				{ x: x + width, y: y + height, color: color_lr },
			]));
		shape.draw(surface);
	},

	drawSolidTriangle(surface, x1, y1, x2, y2, x3, y3, color1, color2, color3)
	{
		color2 = color2 || color1;
		color3 = color3 || color1;

		var shape = new Shape(ShapeType.Triangles,
			new VertexList([
				{ x: x1, y: y1, color: color1 },
				{ x: x2, y: y2, color: color2 },
				{ x: x3, y: y3, color: color3 },
			]));
	},

	drawLine(surface, x1, y1, x2, y2, thickness, color1, color2)
	{
		color2 = color2 || color1;

		var shape;
		var xSize = x2 - x1;
		var ySize = y2 - y1;
		var length = Math.sqrt(xSize*xSize + ySize*ySize);
		if (length == 0.0)
			return;
		var tx = 0.5 * thickness * (y2 - y1) / length;
		var ty = 0.5 * thickness * -(x2 - x1) / length;
		shape = new Shape(ShapeType.Fan,
			new VertexList([
				{ x: x1 + tx, y: y1 + ty, color: color1 },
				{ x: x1 - tx, y: y1 - ty, color: color1 },
				{ x: x2 - tx, y: y2 - ty, color: color2 },
				{ x: x2 + tx, y: y2 + ty, color: color2 },
			]));
		shape.draw(surface);
	},

	drawPoint(surface, x, y, color)
	{
		Prim.drawSolidRectangle(surface, x, y, 1, 1, color);
	},

	drawRectangle(surface, x, y, width, height, thickness, color)
	{
		var t = 0.5 * thickness;
		var x1 = x + t;
		var y1 = y + t;
		var x2 = x1 + width - thickness;
		var y2 = y1 + height - thickness;
		var shape = new Shape(ShapeType.TriStrip,
			new VertexList([
				{ x: x1 - t, y: y1 - t, color: color },
				{ x: x1 + t, y: y1 + t, color: color },
				{ x: x2 + t, y: y1 - t, color: color },
				{ x: x2 - t, y: y1 + t, color: color },
				{ x: x2 + t, y: y2 + t, color: color },
				{ x: x2 - t, y: y2 - t, color: color },
				{ x: x1 - t, y: y2 + t, color: color },
				{ x: x1 + t, y: y2 - t, color: color },
				{ x: x1 - t, y: y1 - t, color: color },
				{ x: x1 + t, y: y1 + t, color: color },
			]));
		shape.draw(surface);
	},

	fill(surface, color)
	{
		Prim.drawSolidRectangle(surface, 0, 0, surface.width, surface.height, color);
	},
};

exports = module.exports = Prim;
exports.__esModule = true;
exports.default = exports;
