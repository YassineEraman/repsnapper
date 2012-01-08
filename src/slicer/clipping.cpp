/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2012  martin.dieringer@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "clipping.h"

CL::Clipper Clipping::clpr;

CL::IntPoint Clipping::ClipperPoint(Vector2d v) 
{
  return CL::IntPoint(CL_FACTOR*(v.x+CL_OFFSET),
		      CL_FACTOR*(v.y+CL_OFFSET));
}
Vector2d Clipping::getPoint(CL::IntPoint p) 
{
  return Vector2d(p.X/CL_FACTOR-CL_OFFSET,
		  p.Y/CL_FACTOR-CL_OFFSET);
}

CL::Polygon Clipping::getClipperPolygon(const Poly poly, bool reverse) 
{
  CL::Polygon cpoly(poly.vertices.size());
  for(size_t i=0; i<poly.vertices.size();i++){
    Vector2d P1;
    if (reverse)
      P1 = poly.getVertexCircular(-i); // normally have to reverse, why?
    else 
      P1 = poly.getVertexCircular(i); 
    cpoly[i]=(ClipperPoint(P1));
  }
  // doesn't work...:
  // cerr<< "poly is hole? "<< hole;
  // cerr<< " -- orient=" <<ClipperLib::Orientation(cpoly) << endl;
  // if (ClipperLib::Orientation(cpoly) == hole) 
  //   std::reverse(cpoly.begin(),cpoly.end());
  return cpoly;
}

CL::Polygons Clipping::getClipperPolygons(const vector<Poly> polys, bool reverse) 
{
  ClipperLib::Polygons cpolys(polys.size());
  for (uint i=0; i<polys.size(); i++) 
    {
      cpolys[i] = getClipperPolygon(polys[i],reverse);
    }
  return cpolys;
}

CL::JoinType Clipping::CLType(JoinType type)
{
  switch (type)
    {
    case jsquare: return CL::jtSquare;
    case jmiter:  return CL::jtMiter;
    case jround:  return CL::jtRound;
    }
  return CL::jtMiter; // default
}
CL::PolyType Clipping::CLType(PolyType type)
{
  switch (type)
    {
    case subject: return CL::ptSubject;
    case clip: return CL::ptClip;
    }
  return CL::ptSubject; // default
}

void Clipping::addPoly(const Poly poly, PolyType type)
{
  clpr.AddPolygon(getClipperPolygon(poly),CLType(type));
  lastZ = poly.getZ();
}

void Clipping::addPolys(const vector<Poly> polys, PolyType type)
{
  clpr.AddPolygons(getClipperPolygons(polys),CLType(type));
  double z=0;
  if (polys.size()>0) z= polys.back().getZ();
  lastZ = z;
}


// // return intersection polys
// vector< vector<Vector2d> > Clipping::intersect(const Poly poly1, const Poly poly2) 
// {
//   CL::Polygon cpoly1 = getClipperPolygon(poly1),
//     cpoly2 =  getClipperPolygon(poly2);
//   clpr.Clear();
//   CL::Polygons sol;
//   clpr.AddPolygon(cpoly1,CL::ptSubject);
//   clpr.AddPolygon(cpoly2,CL::ptClip);
//   clpr.Execute(CL::ctIntersection, sol, CL::pftEvenOdd, CL::pftEvenOdd);

//   vector< vector<Vector2d> > result;
//   for(size_t i=0; i<sol.size();i++)
//     {
//       vector<Vector2d> polypoints;
//       CL::Polygon cpoly = sol[i];
//       for(size_t j=0; j<cpoly.size();j++){
// 	polypoints.push_back(Vector2d(cpoly[j].X,cpoly[j].Y));
//       }
//       result.push_back(polypoints);
//     }
//     return result;
// }

// have added Polyons by addPolygon(s)
vector<Poly> Clipping::intersect()
{
  CL::Polygons inter;
  clpr.Execute(CL::ctIntersection, inter, 
	       CL::pftNonZero, CL::pftNonZero);
  return getPolys(inter, lastZ);
}

// have added Polyons by addPolygon(s)
vector<Poly> Clipping::unite()
{
  CL::Polygons united;
  // CL::Polygons emptypolys;
  // clpr.AddPolygons(emptypolys, CLType(clip));
  clpr.Execute(CL::ctDifference, united, 
	       CL::pftNonZero, CL::pftNonZero);
  return getPolys(united, lastZ);  
}

// have added Polyons by addPolygon(s)
vector<Poly> Clipping::substract()
{
  CL::Polygons diff;
  clpr.Execute(CL::ctDifference, diff, 
	       CL::pftEvenOdd, CL::pftEvenOdd);
  return getPolys(diff, lastZ);
}
vector<Poly> Clipping::substractMerged()
{
  CL::Polygons diff;
  clpr.Execute(CL::ctDifference, diff, 
	       CL::pftEvenOdd, CL::pftEvenOdd);
  return getPolys(getMerged(diff), lastZ);
}

Poly Clipping::getOffset(Poly poly, double distance, JoinType jtype, double miterdist)
{
  CL::Polygons cpolys; cpolys.push_back(getClipperPolygon(poly,true));
  CL::Polygons offset = CLOffset(cpolys, CL_FACTOR*distance, CLType(jtype), miterdist);
  return getPoly((CL::Polygon)offset.front(), poly.getZ());
}
vector<Poly> Clipping::getOffset(vector<Poly> polys, double distance, JoinType jtype,
				 double miterdist)
{
  CL::Polygons cpolys = getClipperPolygons(polys,true);
  CL::Polygons offset = CLOffset(cpolys, CL_FACTOR*distance, CLType(jtype), miterdist);
  double z=0;
  if (polys.size()>0) z= polys.back().getZ();
  return getPolys(offset,z);
}

// offset with reverse test
CL::Polygons Clipping::CLOffset(CL::Polygons cpolys, int cldist, 
				CL::JoinType cljtype, double miter_limit)
{
  CL::Polygons opolys;
  CL::OffsetPolygons(cpolys, opolys, cldist, cljtype, miter_limit);
  return opolys;
}

// overlap a bit and unite to merge adjacent polys
vector<Poly> Clipping::getMerged(vector<Poly> polys) 
{
  //return polys;
  //CL::Clipper clpr;
  CL::Polygons cpolys = getClipperPolygons(polys,true);
  //return getPolys(cpolys,polys.back().getZ(),true);
  // make wider to get overlap
  CL::Polygons offset;
  offset = CLOffset(cpolys, 2, CL::jtMiter, 1);
  //CL::OffsetPolygons(cpolys, offset, 10, ClipperLib::jtMiter, 1);
  //return getPolys(offset, polys.back().getZ(),true);
  clpr.Clear();
  clpr.AddPolygons(offset, CL::ptSubject);
  // CL::Polygons emptypolys;
  // clpr.AddPolygons(emptypolys, CL::ptClip);
  // unite
  CL::Polygons cpolys3;
  clpr.Execute(CL::ctUnion, cpolys3, 
	       CL::pftPositive, CL::pftNegative);
  //cerr << cpolys3.size() << " - "<<offset.size() << endl;
  // shrink the result  
  //CL::OffsetPolygons(cpolys3, offset, -2, ClipperLib::jtMiter, 1);
  //return getPolys(cpolys3,polys.back().getZ(),true);
  offset = CLOffset(cpolys3, -2, CL::jtMiter, 1);
  double z=0;
  if (polys.size()>0) z= polys.back().getZ();
  return getPolys(offset, z);
}

CL::Polygons Clipping::getMerged(CL::Polygons cpolys) 
{
  CL::Polygons offset;
  offset = CLOffset(cpolys, 2, CL::jtMiter, 1);
  clpr.Clear();
  clpr.AddPolygons(offset, CL::ptSubject);
  CL::Polygons cpolys3;
  clpr.Execute(CL::ctUnion, cpolys3, 
	       CL::pftPositive, CL::pftNegative);
  return CLOffset(cpolys3, -2, CL::jtMiter, 1);
}

Poly Clipping::getPoly(const CL::Polygon cpoly, double z) 
{
  Poly p = Poly(z);
  p.vertices.clear();
  uint count = cpoly.size();
  p.vertices.resize(count);
  for (uint i = 0 ; i<count;  i++) { 
    Vector2d v = getPoint(cpoly[i]);
    p.vertices[count-i-1] = v;
  }
  p.cleanup();
  p.calcHole();
  return p;
}

vector<Poly> Clipping::getPolys(const CL::Polygons cpolys, double z) 
{
  uint count = cpolys.size();
  vector<Poly> polys(count);
  for (uint i = 0 ; i<count;  i++) 
    polys[i] = getPoly(cpolys[i], z);
  return polys;
}