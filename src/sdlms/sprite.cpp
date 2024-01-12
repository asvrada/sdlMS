#include "sdlms/sprite.hpp"

Sprite::Sprite(SDL_Texture *texture, SDL_Rect *rect, int format) : _texture(texture),
                                                                   _rect(rect),
                                                                   _format(format)
{
    _camera = Camera::current();
    _graphics = Graphics::current();
}

void Sprite::draw()
{
    SDL_Rect rect{_rect->x, _rect->y, _rect->w, _rect->h};
    rect.x -= _camera->viewport.x;
    rect.y -= _camera->viewport.y;
    _graphics->blitSurface(_texture, NULL, &rect);
}