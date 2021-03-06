This is the server for very simple multiplayer space shooter game.

Players can shoot, rotate and accelerate forwards and backwards. The objective of the game is to score the most points by shooting down other players' spaceships. The game world is toroidal, i.e. everything wraps around the edges. There is a speed limit that projectiles move at and players can get arbitrarily close to (similarly to relativistic physics). Projectiles have a finite lifetime.

## Client

The client was written by my classmates, who unfortunately decided not to publish it. Here's a screenshot:

![A screenshot of the game being played by 4 people.](screenshot.png)

## Usage

- `make` \
  Build the server in the `build` directory (requires Make and CMake).

- `make run` \
  Run the server.

- `make clean` \
  Delete the `build` directory.

## License

    Copyright 2016-2018 Paweł Kraśnicki.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
