#ifndef SUPLA_WEB_PAGE_SERVO_H
#define SUPLA_WEB_PAGE_SERVO_H

#if defined(SUPLA_SERVO_CUSTOM)

#define PATH_SERVO "servo"

void createWebPageServo();
void handlePageServo(int save = 0);
void handlePageServoSave();

#endif // SUPLA_SERVO_CUSTOM

#endif // SUPLA_WEB_PAGE_SERVO_H