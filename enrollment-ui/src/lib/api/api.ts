export * from './enrollment.service';
import { EnrollmentService } from './enrollment.service';
export * from './server.service';
import { ServerService } from './server.service';
export const APIS = [EnrollmentService, ServerService];
