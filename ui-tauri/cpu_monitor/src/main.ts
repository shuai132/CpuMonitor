import {createApp} from "vue";
import App from "./App.vue";

import Toast, {TYPE} from "vue-toastification";
import "vue-toastification/dist/index.css";

import Varlet from '@varlet/ui'
import '@varlet/ui/es/style'

const app = createApp(App);

const options = {
    toastDefaults: {
        [TYPE.ERROR]: {
            timeout: 3000,
            closeButton: false,
            hideProgressBar: true,
        },
        [TYPE.SUCCESS]: {
            timeout: 3000,
            closeButton: false,
            hideProgressBar: true,
        }
    }
};
app.use(Toast, options);

app.use(Varlet);

app.mount("#app");
