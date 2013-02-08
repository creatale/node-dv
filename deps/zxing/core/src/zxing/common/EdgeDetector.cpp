/*
 *  EdgeDetector.cpp
 *  zxing
 *
 *  Created by Ralf Kistner on 7/12/2009.
 *  Copyright 2008 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zxing/common/EdgeDetector.h>
#include <zxing/common/detector/math_utils.h>
#include <algorithm>
#include <cmath>

using namespace std;

namespace zxing {
namespace EdgeDetector {

//* 2012-05-08 hfn "INFINITY": s. NaNny.h

void findEdgePoints(std::vector<Point>& points, const BitMatrix& image, Point start, Point end, bool invert, int skip, float deviation) {
  float xdist = end.x - start.x;
  float ydist = end.y - start.y;
  float length = sqrt(xdist * xdist + ydist * ydist);


  int var;

  //* 2012-04-08 hfn fabs statt abs
  if (fabs(xdist) > fabs(ydist)) {
    // Horizontal
    if (xdist < 0)
      skip = -skip;

    var = int(fabs(deviation * length / xdist));

    float dy = ydist / xdist * skip;
    bool left = (skip < 0) ^ invert;
    int x = int(start.x);

    int steps = int(xdist / skip);
    for (int i = 0; i < steps; i++) {
      x += skip;
      if (x < 0 || x >= (int)image.getWidth())
        continue; // In case we start off the edge
      int my = int(start.y + dy * i);
      int ey = Minimum(my + var + 1, (int)image.getHeight() - 1);
      int sy = Maximum(my - var, 0);
      for (int y = sy + 1; y < ey; y++) {
        if (left) {
          if (image.get(x, y) && !image.get(x, y + 1)) {
            points.push_back(Point((float)x, y + 0.5f));
          }
        } else {
          if (!image.get(x, y) && image.get(x, y + 1)) {
            points.push_back(Point((float)x, y + 0.5f));
          }
        }
      }
    }
  } else {
    // Vertical
    if (ydist < 0)
      skip = -skip;

    var = int(fabs(deviation * length / ydist));

    float dx = xdist / ydist * skip;
    bool down = (skip > 0) ^ invert;
    int y = int(start.y);

    int steps = int(ydist / skip);
    for (int i = 0; i < steps; i++) {
      y += skip;
      if (y < 0 || y >= (int)image.getHeight())
        continue; // In case we start off the edge
      int mx = int(start.x + dx * i);
      int ex = Minimum(mx + var + 1, (int)image.getWidth() - 1);
      int sx = Maximum(mx - var, 0);
      for (int x = sx + 1; x < ex; x++) {
        if (down) {
          if (image.get(x, y) && !image.get(x + 1, y)) {
            points.push_back(Point(x + 0.5f, y + 0.0f));
          }

        } else {
          if (!image.get(x, y) && image.get(x + 1, y)) {
            points.push_back(Point(x + 0.5f, y + 0.0f));
          }
        }

      }
    }

  }
}

Line findLine(const BitMatrix& image, Line estimate, bool invert, int deviation, float threshold, int skip) {
  float t = threshold * threshold;

  Point start = estimate.start;
  Point end = estimate.end;

  vector<Point> edges;
  edges.clear();
  findEdgePoints(edges, image, start, end, invert, skip, deviation);

  int n = edges.size();

  float xdist = end.x - start.x;
  float ydist = end.y - start.y;

  bool horizontal = fabs(xdist) > fabs(ydist);

  float max = 0;
  Line bestLine(start, end);  // prepopulate with the given line, in case we can't find any line for some reason

  for (int i = -deviation; i < deviation; i++) {
    float x1, y1;
    if (horizontal) {
      y1 = start.y + i;
      x1 = start.x - i * ydist / xdist;
    } else {
      y1 = start.y - i * xdist / ydist;
      x1 = start.x + i;
    }

    for (int j = -deviation; j < deviation; j++) {
      float x2, y2;
      if (horizontal) {
        y2 = end.y + j;
        x2 = end.x - j * ydist / xdist;
      } else {
        y2 = end.y - j * xdist / ydist;
        x2 = end.x + j;
      }

      float dx = x1 - x2;
      float dy = y1 - y2;
      float length = sqrt(dx * dx + dy * dy);

      float score = 0;

      for(int k = 0; k < n; k++) {
        const Point& edge = edges[k];
        float dist = ((x1 - edge.x) * dy - (y1 - edge.y) * dx) / length;
        // Similar to least squares method
        float s = t - dist * dist;
        if (s > 0)
          score += s;
      }

      if (score > max) {
        max = score;
        bestLine.start = Point(x1, y1);
        bestLine.end = Point(x2, y2);
      }
    }
  }

  return bestLine;
}

Point intersection(Line a, Line b) {
  float dxa = a.start.x - a.end.x;
  float dxb = b.start.x - b.end.x;
  float dya = a.start.y - a.end.y;
  float dyb = b.start.y - b.end.y;

  float p = a.start.x * a.end.y - a.start.y * a.end.x;
  float q = b.start.x * b.end.y - b.start.y * b.end.x;
  float denom = dxa * dyb - dya * dxb;
  if(fabs(denom) < 1e-12)  // Lines don't intersect (2012-10-10 hfn replaces "denom == 0")
    return Point(INFINITY, INFINITY);

  float x = (p * dxb - dxa * q) / denom;
  float y = (p * dyb - dya * q) / denom;

  return Point(x, y);
}

} // namespace EdgeDetector


} // namespace zxing
