/**
 * SKIDATA Face Recognition Box Interface Description
 * SKIDATA Face Recognition Box provides this API for web clients.
 *
 * OpenAPI spec version: 1.0.0
 * 
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */
import { KeyValues } from './keyValues';


export interface Enrollment { 
    /**
     * reference id of the enrollment in the face recognition system
     */
    referenceId: string;
    /**
     * name of person enrolled
     */
    name: string;
    /**
     * number of images associated with the person enrolled
     */
    images?: number;
    keyValues?: KeyValues;
}
