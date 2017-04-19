// Global resolution for round edges
fn=100;

// See instructions here:
// https://sites.google.com/site/handmadewoodentrains/home/articles

// Diameters:
//   Thomas: 19.4
//   Thomas Early Engineers: 24
//   Imaginarium: 20
//   BigJigs: 21.8
//   Brio: 20.4
radius=10;

// Wheel Thickness:
// Note: Most wheels have a slight slant to them (.5mm or so), and were measured from the thicker end of the slant)
//   Thomas: 2.5 collar,  3 - 4 - 7
//   Thomas Early Engineers: 1.8 collar, 3.7 - 4.2 - 7.7
//   Imaginarium: 2.8 - 5
//   BigJigs: 3.2 - 6
//   Brio: 3 - 5
thickness = 3;
bevel = 1;
hub_thickness = 4;

// Note: axel width
// Thomas (all): 15mm (which is why the wheels need collars)
// Everyone else: 20mm

// Hub Radius
// Based on a variety of Hillman's furniture tacks 10.7-11.8mm diameter, plus some wiggle
hub_radius = 6;

// Hub Collar
// This thickness of the wall that is hub_thickness thick
// measurement of 1.7mm is standard across non-Thomas wheels (Thomas wheels don't have a hub cap)
hub_collar = 1.7;

// Axel radius is based on 3/32" tubing size measuring 2.25mm outside diameter, with a little wiggle room
axel_radius = 1.13;

module wheel() {
	difference() {
		union() {
			rotate_extrude(height = 10, center = true, convexity = 10, twist = 0, $fn=fn)
				polygon(points=[[0,0],[radius,0],[radius,thickness-bevel],[0,thickness]], paths=[[0,1,2,3,0]]);
			cylinder(h = hub_thickness, r = hub_radius+hub_collar, center = false, $fn=fn);
		}
		union() {
			translate([0,0,thickness-bevel])
				cylinder(h = hub_thickness, r = hub_radius, center = false, $fn=fn);
			translate([0,0,-.1])
				cylinder(h = hub_thickness + .1, r = axel_radius, $fn=fn);
		}
	}
}


wheel();
