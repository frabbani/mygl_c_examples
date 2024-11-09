#pragma once

#include "defs.h"

struct Vector2{
  union{
    struct{
      float x, y;
    };
    float xy[2];
  };
  Vector2(){ x = y = 0.0f; }
  Vector2( float x, float y ){
    this->x = x;
    this->y = y;
  }

  bool operator < ( const Vector2& rhs ) const;

  bool operator > ( const Vector2& rhs ) const;

  bool operator == ( const Vector2& rhs ) const;

  bool equals( const Vector2& rhs, float tolerence ) const;

};



struct Vector3{
  union{
    struct{
      float x, y, z;
    };
    float xyz[3];
  };

  Vector3(){ x = y = z = 0.0f;  }

  Vector3( float x, float y, float z ){
    this->x = x;
    this->y = y;
    this->z = z;
  }
  Vector3( const Vector2 &v, float z ){
    this->x = v.x;
    this->y = v.y;
    this->z = z;
  }
  Vector3( float x, const Vector2 &v ){
    this->x = x;
    this->y = v.x;
    this->z = v.y;
  }

  bool isValid() const;

  Vector3 operator + ( const Vector3& rhs ) const;
  Vector3 operator - ( const Vector3& rhs ) const;
  Vector3 operator * ( float s ) const;

  void operator += ( const Vector3& rhs );
  void operator -= ( const Vector3& rhs );
  void operator *= ( float s );
  void normalize();

  bool operator < ( const Vector3& rhs ) const;

  bool operator > ( const Vector3& rhs ) const;

  bool operator == ( const Vector3& rhs ) const;

  Vector3 cross( const Vector3& rhs ) const;

  float dot( const Vector3& rhs ) const;
  float length() const;
  Vector3 normalized() const;

  bool equals( const Vector3& rhs, float tolerence ) const;

};

inline Vector3 operator * ( float s, const Vector3& v ) { return v * s;  }

