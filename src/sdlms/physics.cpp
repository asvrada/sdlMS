#include "sdlms/physics.hpp"
#include <ranges>
#include <utility>
#include <algorithm>

Physics::Physics()
{
    _map = Map::current();
    _character = Character::current();
    _input = Input::current();
}

void Physics::update(int elapsedTime)
{
    float _elapsedTime = elapsedTime / 1000.0;

    _character->_hforce = 0;
    _character->_vforce = 2000;

    if (_input->isKeyHeld(SDL_SCANCODE_LEFT) == true)
    {
        if (_physic_status[GROUND] == true)
        {
            _character->_hforce -= 1400;
            _character->switch_type(Character::Type::WALK1);
        }
        else
        {
            _character->_hforce -= 10;
        }
        _character->_direct = true;
    }
    if (_input->isKeyHeld(SDL_SCANCODE_RIGHT) == true)
    {
        if (_physic_status[GROUND] == true)
        {
            _character->_hforce += 1400;
            _character->switch_type(Character::Type::WALK1);
        }
        else
        {
            _character->_hforce += 10;
        }

        _character->_direct = false;
    }
    if (_input->isKeyHeld(SDL_SCANCODE_UP) == true)
    {
        if (_physic_status[PHYSIC_STATUS::CLIMB] == true)
        {
            // 在梯子上且按住上键
            _character->_vspeed = -100;
            auto y = _character->_pos.y() + _character->_vspeed * _elapsedTime;
            if (y <= std::min(_lp._y1, _lp._y2))
            {
                if (_lp._uf == 1)
                {
                    y = std::min(_lp._y1, _lp._y2);
                    _character->_hspeed = 0;
                    auto fhs = _map->_foothold;
                    for (const auto &[id, it] : fhs | std::views::filter([](const auto &pair)
                                                                         {
                                                                             return pair.second._type != FootHold::WALL; // 后处理与墙面,斜坡碰撞
                                                                         }))
                    {

                        if (_character->_pos.x() != std::clamp(_character->_pos.x(), (float)it._a.x(), (float)it._b.x()))
                        {
                            continue;
                        }

                        Point<float> p1 = {(float)it._a.x(), (float)it._a.y()};
                        Point<float> p2 = {(float)it._b.x(), (float)it._b.y()};

                        std::pair<const Point<float> &, const Point<float> &> l1(_character->_pos,
                                                                                 Point<float>{_character->_pos.x(), y - 5});
                        std::pair<const Point<float> &, const Point<float> &> l2(p1, p2);

                        auto r = segmentsIntersection(l1, l2);
                        if (r.has_value())
                        {
                            // 修改坐标为交点
                            auto intersect_pos = r.value();
                            y = intersect_pos.y();
                            _fh = it;
                            _physic_status[PHYSIC_STATUS::GROUND] = true;
                            _physic_status[PHYSIC_STATUS::CLIMB] = false;
                            _character->switch_type(Character::Type::STAND1);
                            break;
                        }
                    }
                }
                else
                {
                    y = std::min(_lp._y1, _lp._y2);
                }
            }

            _character->_pos.set_y(y);
            _character->_vspeed = 0;
            return;
        }
        else
        {
            // 按住上方向键,判断是否有梯子或者绳子可以爬
            auto lps = _map->_ladderRope;
            auto lp = std::ranges::find_if(lps, [this](const auto &pair)
                                           { return std::abs(this->_character->_pos.x() - pair.first) < 10; });

            // 如果找到符合条件的元素，则输出
            if (lp != lps.end() &&
                (this->_character->_pos.y() == std::clamp(this->_character->_pos.y(), (float)lp->second._y1, (float)lp->second._y2)))
            {
                _character->_pos.set_x(lp->first);
                // 判断爬的是梯子还是绳子
                if (lp->second._l == 1)
                {
                    // 梯子
                    _character->switch_type(Character::Type::LADDER);
                }
                else
                {
                    // 绳子
                    _character->switch_type(Character::Type::ROPE);
                }
                _lp = lp->second;
                _physic_status[PHYSIC_STATUS::CLIMB] = true;
                _physic_status[PHYSIC_STATUS::GROUND] = false;
                return;
            }
        }
    }

    if (_input->isKeyHeld(SDL_SCANCODE_DOWN) == true)
    {
        if (_physic_status[PHYSIC_STATUS::CLIMB] == true)
        {
            // 在梯子上且按住上键
            _character->_vspeed = 100;
            auto y = _character->_pos.y() + _character->_vspeed * _elapsedTime;
            if (y >= std::max(_lp._y1, _lp._y2))
            {

                y = std::max(_lp._y1, _lp._y2);
                _character->_hspeed = 0;
                _physic_status[PHYSIC_STATUS::CLIMB] = false;
                _physic_status[PHYSIC_STATUS::GROUND] = false;
                _character->switch_type(Character::Type::JUMP);
            }

            _character->_pos.set_y(y);
            _character->_vspeed = 0;
        }
        else
        {
            _character->_hspeed = 0;
            _character->switch_type(Character::Type::PRONE);
        }
        return;
    }
    if (_input->isKeyHeld(SDL_SCANCODE_LALT) == true)
    {
        if (_physic_status[GROUND] == true)
        {
            _character->_vspeed = -555;
        }
    }
    if (_physic_status[PHYSIC_STATUS::CLIMB] == true)
    {
        return;
    }

    auto fhs = _map->_foothold;

    // 垂直方向有力,默认质量为1
    _character->_hacc = _character->_hforce;
    if (_physic_status[PHYSIC_STATUS::GROUND] == true)
    {
        // 摩擦力
        if (std::abs(_character->_hacc) < 800) // 合力小于摩擦力
        {
            if (_character->_hspeed == 0.0)
            {
                // 合力没有超过静摩擦力,且人物速度为0,则无加速度
                _character->_hacc = 0;
            }
            else
            {
                if (_character->_hspeed > 0.0)
                {
                    // 摩擦力左
                    _character->_hacc -= 800;
                    _character->_hacc = std::max(-_character->_hspeed / _elapsedTime, _character->_hacc);
                }
                else
                {
                    // 摩擦力右
                    _character->_hacc += 800;
                    _character->_hacc = std::min(-_character->_hspeed / _elapsedTime, _character->_hacc);
                }
            }
        }
        else // 合力大于摩擦力
        {
            if (_character->_hacc > 0)
            {
                _character->_hacc -= 800;
            }
            else
            {
                _character->_hacc += 800;
            }
        }
    }

    _character->_vacc = _character->_vforce;

    auto _hspeed = _character->_hspeed + _elapsedTime * _character->_hacc;
    auto _vspeed = _character->_vspeed + _elapsedTime * _character->_vacc;

    Point<float> new_pos = _character->_pos + Point<float>{(_character->_hspeed + _hspeed) / 2 * _elapsedTime,
                                                           (_character->_vspeed + _vspeed) / 2 * _elapsedTime};

    _character->_hspeed += _elapsedTime * _character->_hacc;
    _character->_vspeed += _elapsedTime * _character->_vacc;

    _character->_vspeed = std::min(_character->_vspeed, 1200.0f);
    _character->_hspeed = std::clamp(_character->_hspeed, -150.0f, 150.0f);

    // 判断是否有向上的速度,起跳,弹簧
    if (_physic_status[PHYSIC_STATUS::GROUND] == true && new_pos.y() < _character->_pos.y())
    {
        _physic_status[PHYSIC_STATUS::GROUND] = false;
    }

    // 地面碰撞检测
    if (_physic_status[PHYSIC_STATUS::GROUND] == true)
    {
        if (_character->_hforce == 0)
        {
            _character->switch_type(Character::Type::STAND1);
        }

        auto x = new_pos.x();
        auto y = _fh.get_y(x);
        if (!y.has_value())
        {
            // 寻找下一个fh
            FootHold fh = _fh;
            while (x < fh._a.x() && x < fh._b.x())
            {
                if (fh._prev == 0)
                {
                    break;
                }
                fh = fhs[fh._prev];
                if (fh._type == FootHold::WALL)
                {
                    break;
                }
            }
            while (x > fh._a.x() && x > fh._b.x())
            {
                if (fh._next == 0)
                {
                    break;
                }
                fh = fhs[fh._next];
                if (fh._type == FootHold::WALL)
                {
                    break;
                }
            }
            // 切换fh
            if (fh._type == FootHold::WALL)
            {
                y = _character->_pos.y();
                if (y <= fh._a.y() && y <= fh._b.y())
                {
                    _physic_status[PHYSIC_STATUS::GROUND] = false;
                    x = new_pos.x();
                }
                else
                {
                    // 撞墙
                    _character->_hspeed = 0.0f;
                    x = _character->_pos.x();
                }
            }
            else
            {
                // 平地或者斜坡,切换fh,重写计算y值
                _fh = fh;
                if (_fh.get_y(x).has_value())
                {
                    y = _fh.get_y(x).value();
                }
                else
                {
                    _physic_status[PHYSIC_STATUS::GROUND] = false;
                    y = _character->_pos.y();
                }
            }
        }
        _character->_vspeed = 0.0f;
        _character->_pos.set_y(y.value());
        _character->_pos.set_x(x);
        return;
    }
    else // 空中碰撞检测
    {
        _character->switch_type(Character::Type::JUMP);
        // 判断上升还是下降
        auto raise = new_pos.y() < _character->_pos.y() ? true : false;

        for (const auto &[id, it] : fhs | std::views::filter([](const auto &pair)
                                                             {
                                                                 return pair.second._type == FootHold::WALL; // 先处理与墙的碰撞
                                                             }))
        {
            Point<float> p1 = {(float)it._a.x(), (float)it._a.y()};
            Point<float> p2 = {(float)it._b.x(), (float)it._b.y()};

            std::pair<const Point<float> &, const Point<float> &> l1(_character->_pos, new_pos);
            std::pair<const Point<float> &, const Point<float> &> l2(p1, p2);

            auto r = segmentsIntersection(l1, l2);
            if (r.has_value())
            {
                // 修改坐标为交点
                auto intersect_pos = r.value();
                // 从空中撞到墙
                _character->_hspeed = 0.0f;
                intersect_pos.set_x(_character->_pos.x());
                intersect_pos.set_y(new_pos.y());
                new_pos = intersect_pos;
            }
        }
        if (raise == false)
        {
            for (const auto &[id, it] : fhs | std::views::filter([](const auto &pair)
                                                                 {
                                                                     return pair.second._type != FootHold::WALL; // 后处理与墙面,斜坡碰撞
                                                                 }))
            {
                Point<float> p1 = {(float)it._a.x(), (float)it._a.y()};
                Point<float> p2 = {(float)it._b.x(), (float)it._b.y()};

                std::pair<const Point<float> &, const Point<float> &> l1(_character->_pos, new_pos);
                std::pair<const Point<float> &, const Point<float> &> l2(p1, p2);

                auto r = segmentsIntersection(l1, l2);
                if (r.has_value())
                {
                    auto intersect_pos = r.value();
                    _physic_status[PHYSIC_STATUS::GROUND] = true;
                    _character->_vspeed = 0.0f;
                    _character->_hspeed /= 2;
                    _fh = it;
                    new_pos = intersect_pos;
                }
            }
        }
        _character->_pos = new_pos;
    }
    return;
}

// 判断线段是否相交
template <typename T>
inline std::optional<Point<T>> Physics::segmentsIntersection(std::pair<const Point<T> &, const Point<T> &> a, std::pair<const Point<T> &, const Point<T> &> b)
{
    std::optional<Point<T>> intersection;

    auto [p1, p2] = a;
    auto [p3, p4] = b;
    // 快速排斥实验
    if ((p1.x() > p2.x() ? p1.x() : p2.x()) < (p3.x() < p4.x() ? p3.x() : p4.x()) ||
        (p1.y() > p2.y() ? p1.y() : p2.y()) < (p3.y() < p4.y() ? p3.y() : p4.y()) ||
        (p3.x() > p4.x() ? p3.x() : p4.x()) < (p1.x() < p2.x() ? p1.x() : p2.x()) ||
        (p3.y() > p4.y() ? p3.y() : p4.y()) < (p1.y() < p2.y() ? p1.y() : p2.y()))
    {
        return intersection;
    }
    // 跨立实验
    if ((((p1.x() - p3.x()) * (p4.y() - p3.y()) - (p1.y() - p3.y()) * (p4.x() - p3.x())) *
         ((p2.x() - p3.x()) * (p4.y() - p3.y()) - (p2.y() - p3.y()) * (p4.x() - p3.x()))) > 0 ||
        (((p3.x() - p1.x()) * (p2.y() - p1.y()) - (p3.y() - p1.y()) * (p2.x() - p1.x())) *
         ((p4.x() - p1.x()) * (p2.y() - p1.y()) - (p4.y() - p1.y()) * (p2.x() - p1.x()))) > 0)
    {
        return intersection;
    }

    auto x = ((p1.y() - p3.y()) * (p2.x() - p1.x()) * (p4.x() - p3.x()) + p3.x() * (p4.y() - p3.y()) * (p2.x() - p1.x()) - p1.x() * (p2.y() - p1.y()) * (p4.x() - p3.x())) / ((p4.x() - p3.x()) * (p1.y() - p2.y()) - (p2.x() - p1.x()) * (p3.y() - p4.y()));
    auto y = (p2.y() * (p1.x() - p2.x()) * (p4.y() - p3.y()) + (p4.x() - p2.x()) * (p4.y() - p3.y()) * (p1.y() - p2.y()) - p4.y() * (p3.x() - p4.x()) * (p2.y() - p1.y())) / ((p1.x() - p2.x()) * (p4.y() - p3.y()) - (p2.y() - p1.y()) * (p3.x() - p4.x()));

    intersection.emplace(Point<float>{x, y});

    return intersection;
}
