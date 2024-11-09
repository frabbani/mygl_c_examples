#include "myvector.h"

#include <cmath>

const float tol = 1e-8f;
const float tolSq = 1e-16f;
const float NaN = 1.0f / 0.0f;

bool Vector2::operator <( const Vector2 &rhs ) const {
  if( x < rhs.x - tol )
    return true;
  else if( x > rhs.x + tol )
    return false;
  else{
    if( y < rhs.y - tol )
      return true;
    else if( y >= rhs.y + tol )
      return false;
  }
  return false;
}

bool Vector2::operator >( const Vector2 &rhs ) const {
  return rhs < *this;
}

bool Vector2::operator ==( const Vector2 &rhs ) const {
  return fabsf( x - rhs.x ) < tol && fabsf( y - rhs.y ) < tol;
}

bool Vector3::operator <( const Vector3 &rhs ) const {
  if( x < rhs.x - tol )
    return true;
  else if( x > rhs.x + tol )
    return false;
  else{
    if( y < rhs.y - tol )
      return true;
    else if( y > rhs.y + tol )
      return false;
    else{
      if( z < rhs.z - tol )
        return true;
      else if( z >= rhs.z + tol )
        return false;
    }
  }
  return false;
}

bool Vector3::operator >( const Vector3 &rhs ) const {
  return rhs < *this;
}

bool Vector2::equals( const Vector2 &rhs, float tolerence ) const {
  return fabsf( x - rhs.x ) < tolerence && fabsf( y - rhs.y ) < tolerence;
}

bool Vector3::operator ==( const Vector3 &rhs ) const {
  return fabsf( x - rhs.x ) < tol && fabsf( y - rhs.y ) < tol && fabsf( z - rhs.z ) < tol;
}

Vector3 Vector3::cross( const Vector3 &rhs ) const {
  Vector3 r;
  r.xyz[0] = (xyz[1] * rhs.xyz[2] - xyz[2] * rhs.xyz[1]);
  r.xyz[1] = -(xyz[0] * rhs.xyz[2] - xyz[2] * rhs.xyz[0]);
  r.xyz[2] = (xyz[0] * rhs.xyz[1] - xyz[1] * rhs.xyz[0]);
  return r;
}

float Vector3::dot( const Vector3 &rhs ) const {
  return x * rhs.x + y * rhs.y + z * rhs.z;
}

Vector3 Vector3::operator +( const Vector3 &rhs ) const {
  Vector3 r;
  r.x = x + rhs.x;
  r.y = y + rhs.y;
  r.z = z + rhs.z;
  return r;
}

Vector3 Vector3::operator -( const Vector3 &rhs ) const {
  Vector3 r;
  r.x = x - rhs.x;
  r.y = y - rhs.y;
  r.z = z - rhs.z;
  return r;
}

Vector3 Vector3::operator *( float s ) const {
  Vector3 r;
  r.x = x * s;
  r.y = y * s;
  r.z = z * s;
  return r;
}

void Vector3::operator +=( const Vector3 &rhs ) {
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
}

void Vector3::operator -=( const Vector3 &rhs ) {
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;
}

void Vector3::operator *=( float s ) {
  x *= s;
  y *= s;
  z *= s;
}

void Vector3::normalize() {
  *this = this->normalized();
}

float Vector3::length() const {
  float s = x * x + y * y + z * z;
  if( s < tolSq )
    return 0.0f;
  return sqrtf( s );
}

Vector3 Vector3::normalized() const {
  float l = length();
  if( l == 0.0f ){
    return Vector3( NaN, NaN, NaN );
  }
  return (1.0f / l) * *this;
}

bool Vector3::isValid() const {
  return (fabsf( x ) != NaN) && (fabsf( y ) != NaN) && (fabsf( z ) != NaN);
}

bool Vector3::equals( const Vector3 &rhs, float tolerence ) const {
  return fabsf( x - rhs.x ) < tolerence && fabsf( y - rhs.y ) < tolerence && fabsf( z - rhs.z ) < tolerence;
}

