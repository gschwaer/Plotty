// header_only_lib for a 2D vector

typedef struct vector_struct {
  float x; // [mm]
  float y; // [mm]
} Vector;

#define vector_print(vec)    \
{                            \
  Serial.print("Vector ");   \
  Serial.print(#vec);        \
  Serial.print(" = { x=");   \
  Serial.print(vec.x);       \
  Serial.print(", y=");      \
  Serial.print(vec.y);       \
  Serial.println(" }");      \
}

static inline float vector_len(Vector vector)
{
  return sqrt(sq((vector).x) + sq((vector).y));
}

static inline Vector vector_add(Vector v1, Vector v2)
{
  Vector res;
  res.x = v1.x + v2.x;
  res.y = v1.y + v2.y;
  return res;
}

static inline Vector vector_subtract(Vector v1, Vector v2)
{
  Vector res;
  res.x = v1.x - v2.x;
  res.y = v1.y - v2.y;
  return res;
}

static inline Vector vector_scale(Vector v, float scalar)
{
  v.x *= scalar;
  v.y *= scalar;
  return v;
}
