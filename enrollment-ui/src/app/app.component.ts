import { Component, NgModule } from '@angular/core';
import { Subject } from 'rxjs/Subject';
import { Observable } from 'rxjs/Observable';
import { WebcamImage, WebcamInitError, WebcamUtil } from 'ngx-webcam';
import {EnrollmentService} from 'src/lib';
import { ActivatedRoute } from '@angular/router';



@Component({
   selector: 'app-root',
   templateUrl: './app.component.html',
   styleUrls: ['./app.component.css']
})
export class AppComponent {
   title = 'SKIDATA Facepresso';
   public showWebcam = true;
   public errors: WebcamInitError[] = [];

   // latest snapshot
   public webcamImage: WebcamImage = null;
   // name of the person on the image
   public name: String = '';

   constructor(private enrollmentService: EnrollmentService, private route: ActivatedRoute) {}

   public handleImage(webcamImage: WebcamImage): void {
      console.info('received webcam image', webcamImage);
      this.webcamImage = webcamImage;
    }
   // webcam snapshot trigger
   private trigger: Subject<void> = new Subject<void>();

   public triggerSnapshot(): void {
      this.trigger.next();
   }

   public enroll(name: string, webcamImage: WebcamImage) {
      console.info('enroll user:'+name+', image:'+webcamImage.imageAsBase64.substr(0, 20));
      try {
         this.enrollmentService.updateEnrollmentBase64(name, webcamImage.imageAsBase64).subscribe(
            params => {
               console.log(params);
            }
         )
      }
      catch(e) {
         console.log(e)
      }
      console.info('enrolled user:'+name);
   }

   public get triggerObservable(): Observable<void> {
      return this.trigger.asObservable();
   }

}
