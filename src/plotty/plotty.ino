#include <Servo.h>
#include <math.h>

#include "vector.h"

/* manual config */
// setup
#define SERIAL_BAUD                    115200
#define UPPER_ARM_LENGTH_MM                80.0f
#define LOWER_ARM_LENGTH_MM                82.0f
// servos
#define SHOULDER_SERVO_PIN                  3
#define ELBOW_SERVO_PIN                     5
#define WRIST_SERVO_PIN                     6
#define SHOULDER_SERVO_90_DEG_DUTY_CYCLE 1310
#define SHOULDER_SERVO_0_DEG_DUTY_CYCLE  2030
#define ELBOW_SERVO_90_DEG_DUTY_CYCLE    1570
#define ELBOW_SERVO_0_DEG_DUTY_CYCLE      850
#define WRIST_PEN_UP_DUTY_CYCLE          1300
#define WRIST_PEN_DOWN_DUTY_CYCLE        1100
#define WRIST_MOVEMENT_DELAY_MS           100 // delay used after pen up/down
#define G6_SERVO_MOVEMENT_DELAY_MS        200 // delay used after G6 movement
// coordinates
#define SHOULDER_TO_ZERO_X_MM              60.0f // can be changed (volatile) at runtime with G92
#define SHOULDER_TO_ZERO_Y_MM             -50.0f // can be changed (volatile) at runtime with G92
// timing
#define STEP_DURATION_MILLIS              100
/* end */

// constants
#define FLOAT_EQUALITY_EPSILON              0.01f

// calculated
#define map_shoulder(angle) \
(SHOULDER_SERVO_0_DEG_DUTY_CYCLE + ((SHOULDER_SERVO_90_DEG_DUTY_CYCLE - SHOULDER_SERVO_0_DEG_DUTY_CYCLE) / (M_PI/2)) * (angle))
#define map_elbow(angle) \
(ELBOW_SERVO_0_DEG_DUTY_CYCLE + ((ELBOW_SERVO_90_DEG_DUTY_CYCLE - ELBOW_SERVO_0_DEG_DUTY_CYCLE) / (M_PI/2)) * (angle))
#define STEP_DURATION_MINUTES ((1.0f*STEP_DURATION_MILLIS)/60/1000)

static bool shoulder_and_elbow_servo_attached = false, wrist_servo_attached = false;
static Servo shoulder, elbow, wrist;

static Vector position_vector, target_vector, origin_to_shoulder_vector;
static float velocity = 600; // [mm/min] (usually overwritten on the first G0/G1)

static bool force_move = false;

static void deploy_pen(bool active)
{
  if(wrist_servo_attached == false) {
    wrist.attach(WRIST_SERVO_PIN);
    wrist_servo_attached = true;
  }
  if(active == true) {
    wrist.writeMicroseconds(WRIST_PEN_DOWN_DUTY_CYCLE);
  } else {
    wrist.writeMicroseconds(WRIST_PEN_UP_DUTY_CYCLE);
  }
  delay(WRIST_MOVEMENT_DELAY_MS);
}

static void G1()
{
  char param_type;
  while(( param_type = Serial.read() ) != '\n')
  {
    if(param_type == -1 || param_type == ' ')
      continue;

    float value = Serial.parseFloat(SKIP_NONE);
    switch(param_type)
    {
      case 'X':
      {
        target_vector.x = value;
        break;
      }
      
      case 'Y':
      {
        target_vector.y = value;
        break;
      }
  
      case 'Z':
      {
        deploy_pen(value < -1*FLOAT_EQUALITY_EPSILON);
        break;
      }
  
      case 'F':
      {
        velocity = value;
        break;
      }
  
      default:
      {
        Serial.print("G1");
        Serial.print(" Parameter ignored: ");
        Serial.print(param_type);
        Serial.println(value);
        break;
      }
    }
  }
}

// A: Shoulder, B: Elbow, C: Wrist
static void G6()
{
  char param_type;
  while(( param_type = Serial.read() ) != '\n')
  {
    if(param_type == -1 || param_type == ' ')
      continue;

    float value = Serial.parseInt(SKIP_NONE);
    if(value < 900 || value > 2100) {
      Serial.print("Bad Duty Cycle. Must be between 900 and 2100.");
      continue;
    }
    switch(param_type)
    {
      case 'A':
      {
        shoulder.writeMicroseconds(value);
        delay(G6_SERVO_MOVEMENT_DELAY_MS);
        break;
      }
      
      case 'B':
      {
        elbow.writeMicroseconds(value);
        delay(G6_SERVO_MOVEMENT_DELAY_MS);
        break;
      }

      case 'C':
      {
        wrist.writeMicroseconds(value);
        delay(G6_SERVO_MOVEMENT_DELAY_MS);
        break;
      }

      default:
      {
        Serial.print("G6");
        Serial.print(" Parameter ignored: ");
        Serial.print(param_type);
        Serial.println(value);
        break;
      }
    }
  }
}

// There are no endstops, so homing just moves to x=0 / y=0 / pen up.
static void G28()
{
  char param_type;
  bool param_given = false;
  force_move = true;
  while(( param_type = Serial.read() ) != '\n')
  {
    if(param_type == -1 || param_type == ' ')
      continue;

    switch(param_type)
    {
      case 'X':
      {
        target_vector.x = 0.0f;
        param_given = true;
        break;
      }
      
      case 'Y':
      {
        target_vector.y = 0.0f;
        param_given = true;
        break;
      }
  
      case 'Z':
      {
        deploy_pen(false);
        param_given = true;
        break;
      }

      default:
      {
        Serial.print("G28");
        Serial.print(" Parameter ignored: ");
        Serial.println(param_type);
        break;
      }
    }
  }
  if(param_given == false) { // home all
    target_vector.x = 0.0f;
    target_vector.y = 0.0f;
    deploy_pen(false);
  }
}

static void G92()
{
  Vector new_position_vector;
  new_position_vector.x = 0.0f;
  new_position_vector.y = 0.0f;
  char param_type;
  while(( param_type = Serial.read() ) != '\n')
  {
    if(param_type == -1 || param_type == ' ')
      continue;

    float value = Serial.parseFloat(SKIP_NONE);
    switch(param_type)
    {
      case 'X':
      {
        new_position_vector.x = value;
        break;
      }
      
      case 'Y':
      {
        new_position_vector.y = value;
        break;
      }
  
      default:
      {
        Serial.print("G92");
        Serial.print(" Parameter ignored: ");
        Serial.print(param_type);
        Serial.println(value);
        break;
      }
    }
  }
  const Vector correction_vector = vector_subtract(position_vector, new_position_vector);
  origin_to_shoulder_vector = vector_subtract(origin_to_shoulder_vector, correction_vector);
  position_vector = new_position_vector;
  Serial.println("OK");
}

// returns true if the command requires a movement
static bool parse_command()
{
  char cmd_char = Serial.read();
  long code = Serial.parseInt(SKIP_NONE);
  switch(cmd_char)
  {
    case 'G':
    {
      switch(code)
      {
        case 0: case 1: // G0 & G1: Move
        {
          G1();
          return true;
        }

        case 6: // G6: Direct Stepper Move
        {
          G6();
          Serial.println("OK");
          return false;
        }

        case 28: // G28: Move to Origin (Home)
        {
          G28();
          return true;
        }

        case 21: // G21: Set Units to Millimeters (always mm)
        case 90: // G90: Set to Absolute Positioning (always absolute)
        {
          Serial.println("OK");
          goto skip_cmd;
        }

        case 92: // G92: Set Position
        {
          G92();
          return false;
        }
        
        default:
        {
          Serial.print("Unsupported code: ");
          Serial.print(cmd_char);
          Serial.println(code);
          goto skip_cmd;
        }
      }
    }

    case 'M':
    {
      switch(code)
      {
        case 0: // M0: Stop or Unconditional stop (no buffer, just stop sending new data)
        case 1: // M1: Sleep or Conditional stop (no buffer, just stop sending new data)
        {
          Serial.println("OK");
          goto skip_cmd;
        }

        case 2: // M2: Program End (we should always go to the origin on end, so homing after reset is less rapid)
        {
          G28();
          return true;
        }
        
        default:
        {
          Serial.print("Unsupported code: ");
          Serial.print(cmd_char);
          Serial.println(code);
          goto skip_cmd;
        }
      }
    }

    case '\n':
    {
      return false;
    }
    
    default:
    {
      Serial.print("Unsupported type: ");
      Serial.println(cmd_char);
      goto skip_cmd;
    }
  }
  Serial.println("Unexpected Parser Fault!");
  return false;

skip_cmd:
  if(cmd_char != '\n')
    while(Serial.read() != '\n');
  return false;
}

static void move_pen_to(Vector absolute_movement_vector)
{
  // elbow angle and partially shoulder angle depend on the distance from the shoulder to the target
  const Vector shoulder_to_target_vector = vector_subtract(absolute_movement_vector, origin_to_shoulder_vector);
  const float shoulder_to_target_vector_len = vector_len(shoulder_to_target_vector);

  // calculating angles from the side lengths of a triangle composed of 
  // lower arm, upper arm, line from shoulder to target, using law of cosines
#define sss_a LOWER_ARM_LENGTH_MM
#define sss_b UPPER_ARM_LENGTH_MM
#define sss_c shoulder_to_target_vector_len
  const float sss_alpha = acos( (sq(sss_a) - sq(sss_b) - sq(sss_c)) / (-2 * sss_b * sss_c) );
  const float sss_gamma = acos( (sq(sss_c) - sq(sss_a) - sq(sss_b)) / (-2 * sss_a * sss_b) );
  
  const float shoulder_angle_for_distance = sss_alpha;
  const float elbow_angle = sss_gamma;

  // the other part of the shoulder angle comes from the angle (from the shoulder) to the target
  const float angle_shoulder_to_target = atan(shoulder_to_target_vector.y/shoulder_to_target_vector.x);
  const float shoulder_angle = shoulder_angle_for_distance + angle_shoulder_to_target;

  const int shoulder_duty = map_shoulder(shoulder_angle);
  const int elbow_duty = map_elbow(elbow_angle);

  if(shoulder_and_elbow_servo_attached == false) {
    shoulder.attach(SHOULDER_SERVO_PIN);
    elbow.attach(ELBOW_SERVO_PIN);
    shoulder_and_elbow_servo_attached = true;
  }
  shoulder.writeMicroseconds(shoulder_duty);
  elbow.writeMicroseconds(elbow_duty);
}

void loop()
{
  static bool moving = false;
  static unsigned long next_step_at_millis = 0;
  
  if(moving == true)
  {
    if(millis() >= next_step_at_millis)
    {
      bool was_last_movement = false;
      
      const Vector error_vector = vector_subtract(target_vector, position_vector);
      const float error_vector_len = vector_len(error_vector);
      
      if(error_vector_len < FLOAT_EQUALITY_EPSILON) { // we are already there
        if(force_move == true) { // e.g. on boot if the device thinks its at (0/0) but it isnt (used by G28)
          move_pen_to(target_vector);
        }
        was_last_movement = true;
      } else {
        const float movement_vector_len = velocity * STEP_DURATION_MINUTES;
        const Vector movement_vector = vector_scale(error_vector, (movement_vector_len / error_vector_len));

        Vector absolute_movement_vector;
        if(error_vector.x >= 0.0f){ // target pos is in +x direction
          absolute_movement_vector.x = min(position_vector.x + movement_vector.x, target_vector.x);
        } else { // target pos is in -x direction
          absolute_movement_vector.x = max(position_vector.x + movement_vector.x, target_vector.x);
        }
        if(error_vector.y >= 0.0f){ // target pos is in +y direction
          absolute_movement_vector.y = min(position_vector.y + movement_vector.y, target_vector.y);
        } else { // target pos is in -y direction
          absolute_movement_vector.y = max(position_vector.y + movement_vector.y, target_vector.y);
        }

        // potential speed up: saving this vector and length for the next loop iteration if was_last_movement == false
        const Vector next_error_vector = vector_subtract(target_vector, absolute_movement_vector);
        const float next_error_vector_len = vector_len(error_vector);
        if(next_error_vector_len < FLOAT_EQUALITY_EPSILON) {
          was_last_movement = true;
        }
        
        move_pen_to(absolute_movement_vector);
        
        next_step_at_millis += STEP_DURATION_MILLIS;
        position_vector = absolute_movement_vector;
      }

      force_move = false;

      if(was_last_movement == true) {
        Serial.println("OK");
        moving = false;
      }
    }
  } else if(Serial.available() > 0) {
    bool movement_required = parse_command();
    if(movement_required == true) {
      moving = true;
      next_step_at_millis = max(next_step_at_millis + STEP_DURATION_MILLIS, millis());
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  origin_to_shoulder_vector.x = -(SHOULDER_TO_ZERO_X_MM);
  origin_to_shoulder_vector.y = -(SHOULDER_TO_ZERO_Y_MM);
}
