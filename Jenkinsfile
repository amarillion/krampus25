node {

	catchError {
		def TWIST_HOME = "${env.JENKINS_HOME}/workspace/twist5"

		stage('CheckOut') {
			checkout scm
		}

		stage('Build Emscripten') {
			docker.image('amarillion/alleg5-emscripten:latest').inside("-v $TWIST_HOME:$TWIST_HOME") {
				sh "make TARGET=EMSCRIPTEN BUILD=RELEASE TWIST_HOME=$TWIST_HOME"
			}
		}

		stage('Build Linux') {
			docker.image('amarillion/alleg5-plus-buildenv:latest').inside("-v $TWIST_HOME:$TWIST_HOME") {
				sh "make TARGET=LINUX BUILD=RELEASE TWIST_HOME=$TWIST_HOME"
			}
		}
		
	}

//	mailIfStatusChanged env.EMAIL_RECIPIENTS
	mailIfStatusChanged "mvaniersel@gmail.com"
}

//see: https://github.com/triologygmbh/jenkinsfile/blob/4b-scripted/Jenkinsfile
def mailIfStatusChanged(String recipients) {
    
	// Also send "back to normal" emails. Mailer seems to check build result, but SUCCESS is not set at this point.
    if (currentBuild.currentResult == 'SUCCESS') {
        currentBuild.result = 'SUCCESS'
    }
    step([$class: 'Mailer', recipients: recipients])
}
