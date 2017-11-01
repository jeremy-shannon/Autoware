#include <cmath>
#include <cross_road_area.hpp>
#include <euclidean_space.hpp>

namespace decision_maker
{
#define TARGET_WAYPOINTS_NUM 15  // need to change rosparam
CrossRoadArea *CrossRoadArea::findClosestCrossRoad(const autoware_msgs::lane &_finalwaypoints,
                                                   std::vector<CrossRoadArea> &intersects)
{
  CrossRoadArea *_area = nullptr;

  euclidean_space::point _pa;
  euclidean_space::point _pb;

  double _min_distance = DBL_MAX;

  int _label = 1;

  if (!_finalwaypoints.waypoints.empty())
  {
    _pa.x = _finalwaypoints.waypoints[TARGET_WAYPOINTS_NUM].pose.pose.position.x;
    _pa.y = _finalwaypoints.waypoints[TARGET_WAYPOINTS_NUM].pose.pose.position.y;
    _pa.z = 0.0;
  }

  for (size_t i = 0; i < intersects.size(); i++)
  {
    _pb.x = intersects[i].bbox.pose.position.x;
    _pb.y = intersects[i].bbox.pose.position.y;

    _pb.z = 0.0;

    double __temp_dis = euclidean_space::EuclideanSpace::find_distance(&_pa, &_pb);

    intersects[i].bbox.label = 0;
    if (_min_distance >= __temp_dis)
    {
      _area = &intersects[i];
      _min_distance = __temp_dis;  //
    }
  }

  if (_area)
  {
    _area->bbox.label = 3;
  }

  return _area;
}

std::vector<geometry_msgs::Point> convhull(const CrossRoadArea *_ClosestArea)
{
  std::vector<int> enablePoints;

  // Jarvis's March algorithm
  int l = 0;
  for (auto i = begin(_ClosestArea->points); i != end(_ClosestArea->points); i++)
  {
    if (i->x < _ClosestArea->points.at(l).x)
    {
      l = std::distance(begin(_ClosestArea->points), i);
    }
  }

  int p = l;
  int q;

  do
  {
    q = (p + 1) % _ClosestArea->points.size();
    for (int i = 0; i < _ClosestArea->points.size(); i++)
    {
      geometry_msgs::Point pp = _ClosestArea->points.at(p);
      geometry_msgs::Point pi = _ClosestArea->points.at(i);
      geometry_msgs::Point pq = _ClosestArea->points.at(q);
      if ((pi.y - pp.y) * (pq.x - pi.x) - (pi.x - pp.x) * (pq.y - pi.y) < 0)
      {
        q = i;
      }
    }
    enablePoints.push_back(p);
    p = q;
  } while (p != l);

  std::vector<geometry_msgs::Point> point_arrays;
  for (auto p = begin(_ClosestArea->points); p != end(_ClosestArea->points); p++)
  {
    for (auto &en : enablePoints)
    {
      if (std::distance(begin(_ClosestArea->points), p) == en)
      {
        point_arrays.push_back(*p);
      }
    }
  }
  return point_arrays;
}

bool CrossRoadArea::isInsideArea(const CrossRoadArea *_ClosestArea, geometry_msgs::Point pt)
{
  std::vector<geometry_msgs::Point> point_arrays = convhull(_ClosestArea);

  double rad = 0.0;
  for (auto it = begin(point_arrays); it != end(point_arrays); ++it)
  {
    auto it_n = it + 1;
    if (it == --point_arrays.end())
    {
      it_n = point_arrays.begin();
    }

    double ax = it->x - pt.x;
    double ay = it->y - pt.y;
    double bx = it_n->x - pt.x;
    double by = it_n->y - pt.y;
    double cos_ = (ax * bx + ay * by) / (sqrt(ax * ax + ay * ay) * sqrt(bx * bx + by * by));
    if (cos_ > 1.0)
    {
      cos_ = 1;
    }
    else if (cos_ < -1.0)
    {
      cos_ = -1.0;
    }

    // deg += std::acos(cos_)? std::acos(cos_)/ M_PI * 180.0 : 0.0;
    rad += std::acos(cos_) ? std::acos(cos_) : 0.0;
  }
  if (fabs((2 * M_PI) - rad) <= 0.35 /*about 30 degree*/)
  {
    return true;
  }

  return false;
#if 0
	// Winding Number Algorithm 
	// not working
	int wn = 0;
	double vt;

	double s = 0;

	for(size_t i = _ClosestArea->points.size()-1; i > 0 ; i--){
		auto it = _ClosestArea->points.at(i);
		auto it_n = _ClosestArea->points.at(i-1);
		std::cout << "point[" << i << "]:" << it.x << "," << it.y << std::endl;
		if( (it.y <= pt.y ) && (it_n.y > pt.y)){
			vt = (pt.y - it.y) / (it_n.y - pt.y);
			if(pt.x < it.x + (vt * (it_n.x - it.x))){
				++wn;
			}
		}
		else if( (it.y > pt.y) && (it_n.y <= pt.y)) {
			vt = (pt.y - it.y) / (it_n.y - pt.y);
			if(pt.x < ( it.x + (vt * (it_n.x - it.x)))){
				--wn;
			}
		}

	}
	if(wn > 0 ) return true;
	else return false;

#endif
}
}