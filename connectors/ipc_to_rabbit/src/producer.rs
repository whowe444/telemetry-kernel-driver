use posixmq::PosixMq;

pub trait Producer {
    fn get(&self) -> Vec<u8>;
}

pub struct ProducerImpl {
    mq: PosixMq
}

impl ProducerImpl {
    pub fn new(mq: PosixMq) -> Self {
        Self { mq }
    }
}

impl Producer for ProducerImpl {
    fn get(&self) -> Vec<u8> {
        let mut buf = vec![0u8; self.mq.attributes().unwrap().max_msg_len];
        match self.mq.recv(&mut buf) {
            Ok((_priority, len)) => {
                println!("Received message: {:?}", String::from_utf8_lossy(&buf[..len]));
                return buf[..len].to_vec()
            },
            Err(e) => {
                eprintln!("Error receiving message: {}", e);
                return buf[..0].to_vec();
            }
        }
    }
}